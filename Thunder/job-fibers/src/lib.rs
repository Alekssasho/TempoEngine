use std::{
    cell::RefCell,
    ffi::c_void,
    sync::atomic::{AtomicBool, AtomicU32},
};

use windows::Win32::System::Threading as wapi;

pub trait JobContext {}

pub type JobEntryPoint = fn(u32, &dyn JobContext);
pub struct JobDecl {
    pub entry_point: JobEntryPoint,
    pub data: *const dyn JobContext,
}
pub struct Counter(AtomicU32);

pub enum ThreadTag {
    Worker,
    Windows,
    Count,
}

struct JobData {
    job: JobDecl,
    counter: Option<*const Counter>,
    name: &'static str,
    index: u32,
}

struct ReadyFiber {
    fiber_id: FreeFiber,
    job_name: &'static str,
}

struct WaitingFiber {
    fiber_id: FreeFiber,
    target_value: u32,
    job_name: &'static str,
    can_be_made_ready: AtomicBool,
}

struct ThreadQueueReceivers {
    jobs: crossbeam::channel::Receiver<JobData>,
    ready_fibers: crossbeam::channel::Receiver<ReadyFiber>,
}

pub struct JobSystem {
    quit_flag: AtomicBool,
    fibers: Vec<*mut c_void>,
    worker_threads: Vec<std::thread::JoinHandle<()>>,
    free_fibers: crossbeam::channel::Sender<FreeFiber>,
    waiting_fibers: std::sync::Mutex<multimap::MultiMap<*const Counter, WaitingFiber>>,
}

#[derive(Clone, Copy)]
struct FreeFiber {
    handle: *mut c_void,
    index: u32,
}

unsafe impl Send for FreeFiber {}

unsafe impl Send for JobSystem {}
unsafe impl Sync for JobSystem {}

impl JobSystem {
    pub fn new(num_worker_threads: usize, num_fibers: usize, fiber_stack_size: usize) -> JobSystem {
        let (sender, receiver) = crossbeam::channel::unbounded();
        let mut fibers = Vec::with_capacity(num_fibers);
        let mut system = JobSystem {
            quit_flag: AtomicBool::new(false),
            fibers: Vec::new(),
            worker_threads: Vec::new(),
            free_fibers: sender,
            waiting_fibers: std::sync::Mutex::new(multimap::MultiMap::new()),
        };
        for i in 0..num_fibers {
            fibers.push(unsafe {
                wapi::CreateFiber(
                    fiber_stack_size,
                    Some(JobSystem::fiber_entry_point),
                    &mut system as *mut _ as *mut c_void,
                )
            });
            system.free_fibers.send(FreeFiber{
                handle: *fibers.last().unwrap(),
                index: i as u32,
            }).unwrap();
        }

        if num_worker_threads > 0 {
            let mut worker_threads = Vec::with_capacity(num_worker_threads);
            let thread_data = WorkerThreadData {
                free_fibers: receiver.clone(),
            };
            worker_threads.push(
                std::thread::Builder::new()
                    .name("Worker Thread".to_string())
                    .spawn(move || {
                        JobSystem::worker_thread_entry_point(thread_data, ThreadTag::Windows)
                    })
                    .unwrap(),
            );

            for _ in 1..num_worker_threads {
                let thread_data = WorkerThreadData {
                    free_fibers: receiver.clone(),
                };
                worker_threads.push(
                    std::thread::Builder::new()
                        .name("Worker Thread".to_string())
                        .spawn(move || {
                            JobSystem::worker_thread_entry_point(thread_data, ThreadTag::Worker)
                        })
                        .unwrap(),
                );
            }
            system.worker_threads = worker_threads;
        }

        system.fibers = fibers;
        system
    }

    pub fn quit(&self) {
        self.quit_flag
            .store(true, std::sync::atomic::Ordering::SeqCst);
    }

    pub fn wait_for_completion(&mut self) {
        for thread in self.worker_threads.drain(..) {
            thread.join().unwrap();
        }
    }

    unsafe extern "system" fn fiber_entry_point(params: *mut c_void) {
        let system = &*(params as *mut JobSystem);

        system.cleanup_old_fiber();

        while !system.quit_flag.load(std::sync::atomic::Ordering::SeqCst) {
            // TODO: add additional queues
            //let did_we_run = JobSystem::fiber_loop_body(system, )
            JobSystem::fiber_loop_body(system);
        }

        let mut initial_fiber = std::ptr::null_mut();
        WORKER_THREAD_DATA.with(|tls| {
            let tls = tls.borrow();
            initial_fiber = tls.initial_fiber;
        });
        wapi::SwitchToFiber(initial_fiber);

        panic!("This should not be reached");
    }


    fn worker_thread_entry_point(worker_thread_data: WorkerThreadData, tag: ThreadTag) {
        let initial_fiber = unsafe { wapi::ConvertThreadToFiber(std::ptr::null()) };
        WORKER_THREAD_DATA.with(|tls| {
            let mut tls = tls.borrow_mut();
            tls.tag = tag;
            tls.initial_fiber = initial_fiber;
        });

        let FreeFiber { handle, index } = worker_thread_data.free_fibers.recv().unwrap();

        WORKER_THREAD_DATA.with(|tls| {
            let mut tls = tls.borrow_mut();
            tls.current_fiber_id.index = index;
        });

        unsafe { wapi::SwitchToFiber(handle) };

        unsafe {
            wapi::ConvertFiberToThread();
        }
    }

    fn cleanup_old_fiber(&self) {
        WORKER_THREAD_DATA.with(|tls| {
            let mut tls = tls.borrow_mut();
            if tls.fiber_to_push_to_free_list.index != u32::MAX {
                self.free_fibers.send(tls.fiber_to_push_to_free_list).unwrap();
            } else if let Some(flag) = tls.can_be_made_ready_flag {
                let flag = unsafe{ &*flag };
                flag.store(true, std::sync::atomic::Ordering::SeqCst);
                tls.can_be_made_ready_flag = None;
            }
        });
    }

    fn fiber_loop_body(&self, job_queue: &ThreadQueueReceivers) -> bool {
        if !job_queue.ready_fibers.is_empty() {
            let ready_fiber = job_queue.ready_fibers.recv().unwrap();
            WORKER_THREAD_DATA.with(|tls| {
                let mut tls = tls.borrow_mut();

                tls.fiber_to_push_to_free_list = tls.current_fiber_id;

                tls.current_job_name = ready_fiber.job_name;
                tls.current_fiber_id = ready_fiber.fiber_id;
            });

            unsafe{ wapi::SwitchToFiber(ready_fiber.fiber_id.handle)};

            self.cleanup_old_fiber();

            return true;
        } else if !job_queue.jobs.is_empty() {
            let job_data = job_queue.jobs.recv().unwrap();
            WORKER_THREAD_DATA.with(|tls| {
                let mut tls = tls.borrow_mut();

                tls.current_job_name = job_data.name;
            });

            (job_data.job.entry_point)(job_data.index, unsafe{&*job_data.job.data});
            WORKER_THREAD_DATA.with(|tls| {
                let mut tls = tls.borrow_mut();

                tls.current_job_name = "";
            });

            if let Some(counter) = job_data.counter {
                let waiting_fibers = self.waiting_fibers.lock().unwrap();

                unsafe{ (*counter).0.fetch_sub(1, std::sync::atomic::Ordering::SeqCst) };

                for fiber in waiting_fibers.get_vec(&counter).unwrap() {
                    if unsafe{ (*counter).0.load(std::sync::atomic::Ordering::SeqCst) } <= fiber.target_value {
                        while !fiber.can_be_made_ready.load(std::sync::atomic::Ordering::SeqCst) {}

                        
                    }
                }
            }
        }

        false
    }
}

impl Drop for JobSystem {
    fn drop(&mut self) {
        self.wait_for_completion();

        for fiber in self.fibers.drain(..) {
            unsafe{ wapi::DeleteFiber(fiber); }
        }
    }
}

struct WorkerThreadData {
    free_fibers: crossbeam::channel::Receiver<FreeFiber>,
}

struct ThreadLocalData {
    current_job_name: &'static str,
    initial_fiber: *mut c_void,
    current_fiber_id: FreeFiber,
    fiber_to_push_to_free_list: FreeFiber,
    can_be_made_ready_flag: Option<*const AtomicBool>,
    tag: ThreadTag,
}

// TODO: Fiber safe optimization
thread_local! {
    static WORKER_THREAD_DATA: RefCell<ThreadLocalData>  = RefCell::new(ThreadLocalData{
        current_job_name: "",
        initial_fiber: std::ptr::null_mut(),
        current_fiber_id: FreeFiber{ handle: std::ptr::null_mut(), index: u32::MAX },
        fiber_to_push_to_free_list: FreeFiber{ handle: std::ptr::null_mut(), index: u32::MAX },
        can_be_made_ready_flag: None,
        tag: ThreadTag::Worker,
    });
}

#[cfg(test)]
mod tests {
    use crate::JobSystem;

    #[test]
    fn it_works() {
        let mut job_system = JobSystem::new(8, 64, 10000000);

        job_system.run_jobs();
        job_system.wait_for_completion();
    }
}
