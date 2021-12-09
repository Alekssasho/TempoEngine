use std::{
    ffi::c_void,
    sync::atomic::{AtomicBool, AtomicU32},
};
use winapi::um::winbase as wapi;

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
}

unsafe impl Send for JobSystem{}
unsafe impl Sync for JobSystem{}

impl JobSystem {
    pub fn new(
        num_workder_threads: usize,
        num_fibers: usize,
        fiber_stack_size: usize,
    ) -> JobSystem {
        let mut fibers = Vec::with_capacity(num_fibers);
        let mut system = JobSystem {
            quit_flag: AtomicBool::new(false),
            fibers: Vec::new(),
            worker_threads: Vec::new(),
        };
        for _ in 0..num_fibers {
            fibers.push(unsafe {
                wapi::CreateFiber(
                    fiber_stack_size,
                    Some(JobSystem::fiber_entry_point),
                    &mut system as *mut _ as *mut c_void,
                )
            });
        }

        if num_workder_threads > 0 {
            let mut worker_threads = Vec::with_capacity(num_workder_threads);
            unsafe {worker_threads.push(std::thread::spawn(|| JobSystem::worker_thread_entry_point(&system as *const _ as *const c_void, ThreadTag::Windows)))};


            system.worker_threads = worker_threads;
        }

        system.fibers = fibers;
        system
    }

    unsafe extern "system" fn fiber_entry_point(params: *mut c_void) {}
    fn worker_thread_entry_point(this: *const c_void, tag: ThreadTag) {

    }
}

impl Drop for JobSystem {
    fn drop(&mut self) {}
}

#[cfg(test)]
mod tests {
    #[test]
    fn it_works() {
        assert_eq!(2 + 2, 4);
    }
}
