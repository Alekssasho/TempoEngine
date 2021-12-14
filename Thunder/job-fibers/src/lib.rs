use std::{
    borrow::BorrowMut,
    cell::RefCell,
    ffi::c_void,
    sync::atomic::{AtomicBool, AtomicU32},
};

use windows::Win32::System::Threading as wapi;

pub trait JobContext {}

pub type JobEntryPoint = fn(u32, &dyn JobContext);
pub struct JobDecl {
    pub entry_point: JobEntryPoint,
    pub data: dyn JobContext,
}
pub struct Counter(AtomicU32);

pub enum ThreadTag {
    Worker,
    Windows,
    Count,
}

pub struct JobSystem {
    quit_flag: AtomicBool,
    fibers: Vec<*mut c_void>,
    worker_threads: Vec<std::thread::JoinHandle<()>>,
    free_fibers: crossbeam::channel::Sender<FreeFiber>,
}

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

    unsafe extern "system" fn fiber_entry_point(params: *mut c_void) {}
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
            tls.current_fiber_id = index;
        });

        unsafe { wapi::SwitchToFiber(handle) };

        unsafe {
            wapi::ConvertFiberToThread();
        }
    }
}

impl Drop for JobSystem {
    fn drop(&mut self) {}
}

struct WorkerThreadData {
    free_fibers: crossbeam::channel::Receiver<FreeFiber>,
}

struct ThreadLocalData {
    current_job_name: String,
    initial_fiber: *mut c_void,
    current_fiber_id: u32,
    fiber_to_push_to_free_list: u32,
    can_be_made_ready_flag: AtomicBool,
    tag: ThreadTag,
}

// TODO: Fiber safe optimization
thread_local! {
    static WORKER_THREAD_DATA: RefCell<ThreadLocalData>  = RefCell::new(ThreadLocalData{
        current_job_name: String::new(),
        initial_fiber: std::ptr::null_mut(),
        current_fiber_id: u32::MAX,
        fiber_to_push_to_free_list: u32::MAX,
        can_be_made_ready_flag: AtomicBool::new(false),
        tag: ThreadTag::Worker,
    });
}

#[cfg(test)]
mod tests {
    #[test]
    fn it_works() {
        assert_eq!(2 + 2, 4);
    }
}
