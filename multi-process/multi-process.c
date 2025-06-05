#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/kthread.h>
#include <linux/spinlock.h>
#include <linux/cpumask.h>
#include <linux/smp.h>
#include <linux/delay.h>

#define MAX_WAITER_THREADS 4
static struct task_struct *task_holder_thread;
static struct task_struct *task_waiter_threads[MAX_WAITER_THREADS];
static spinlock_t my_lock;
static int num_waiters = 3;

// Task that holds the lock briefly, then releases it periodically
static int task_holder_func(void *arg)
{
    pr_info("Task Holder: Running on CPU %d\n", smp_processor_id());

    while (!kthread_should_stop())
    {
        spin_lock(&my_lock);
        pr_info("Task Holder: Acquired spinlock on CPU %d\n", smp_processor_id());

        // Hold the lock for a short time to allow waiters to queue up
        msleep(200); // Hold for 100ms

        pr_info("Task Holder: Releasing spinlock on CPU %d\n", smp_processor_id());
        spin_unlock(&my_lock);

        // Brief pause before trying to acquire again
        msleep(10); // 10ms pause
    }

    pr_info("Task Holder: Exiting.\n");
    return 0;
}

// Task that continuously tries to acquire the lock
static int task_waiter_func(void *arg)
{
    int waiter_id = (int)(long)arg;
    int cpu_id = smp_processor_id();

    pr_info("Task Waiter %d: Running on CPU %d. Starting to compete for spinlock.\n",
            waiter_id, cpu_id);

    while (!kthread_should_stop())
    {
        pr_info("Task Waiter %d: Attempting to acquire spinlock on CPU %d\n",
                waiter_id, cpu_id);

        // This is where native_queued_spin_lock_slowpath should be triggered
        // when multiple threads compete for the lock
        spin_lock(&my_lock);

        pr_info("Task Waiter %d: Acquired spinlock on CPU %d!\n",
                waiter_id, cpu_id);

        // Hold the lock briefly
        msleep(50);

        spin_unlock(&my_lock);
        pr_info("Task Waiter %d: Released spinlock on CPU %d\n",
                waiter_id, cpu_id);

        // Brief pause before trying again
        msleep(20);
    }

    pr_info("Task Waiter %d: Exiting.\n", waiter_id);
    return 0;
}

static int softlockup_test_init(void)
{
    int i;
    int available_cpus = num_online_cpus();

    pr_info("Softlockup Test: Initializing module with %d online CPUs.\n", available_cpus);

    // Adjust the number of waiters based on available CPUs
    if (available_cpus >= 4)
    {
        num_waiters = 3;
    }
    else if (available_cpus >= 3)
    {
        num_waiters = 2;
    }
    else
    {
        num_waiters = 1;
        pr_warn("Softlockup Test: Limited CPUs available, may not trigger slowpath effectively.\n");
    }

    pr_info("Softlockup Test: Creating 1 holder + %d waiter threads.\n", num_waiters);

    spin_lock_init(&my_lock);

    // Create holder thread (runs on CPU 0)
    task_holder_thread = kthread_run(task_holder_func, NULL, "softlockup_holder");
    if (IS_ERR(task_holder_thread))
    {
        pr_err("Softlockup Test: Failed to create holder thread: %ld\n", PTR_ERR(task_holder_thread));
        return PTR_ERR(task_holder_thread);
    }

    if (cpu_online(0))
    {
        set_cpus_allowed_ptr(task_holder_thread, cpumask_of(0));
        pr_info("Softlockup Test: Pinned holder thread to CPU 0.\n");
    }

    // Create waiter threads (distribute across other CPUs)
    for (i = 0; i < num_waiters; i++)
    {
        char thread_name[32];
        int target_cpu = (i + 1) % available_cpus;

        snprintf(thread_name, sizeof(thread_name), "softlockup_waiter_%d", i);

        task_waiter_threads[i] = kthread_run(task_waiter_func, (void *)(long)i, thread_name);
        if (IS_ERR(task_waiter_threads[i]))
        {
            pr_err("Softlockup Test: Failed to create waiter thread %d: %ld\n",
                   i, PTR_ERR(task_waiter_threads[i]));
            // Clean up previously created threads
            goto cleanup;
        }

        if (cpu_online(target_cpu))
        {
            set_cpus_allowed_ptr(task_waiter_threads[i], cpumask_of(target_cpu));
            pr_info("Softlockup Test: Pinned waiter thread %d to CPU %d.\n", i, target_cpu);
        }
    }

    pr_info("Softlockup Test: All threads created. Lock contention should trigger slowpath.\n");
    return 0;

cleanup:
    // Clean up any successfully created waiter threads
    for (i = i - 1; i >= 0; i--)
    {
        if (task_waiter_threads[i])
        {
            kthread_stop(task_waiter_threads[i]);
            task_waiter_threads[i] = NULL;
        }
    }

    if (task_holder_thread)
    {
        kthread_stop(task_holder_thread);
        task_holder_thread = NULL;
    }

    return -ENOMEM;
}

static void softlockup_test_exit(void)
{
    int i;

    pr_info("Softlockup Test: Exiting module.\n");

    // Stop all waiter threads first
    for (i = 0; i < num_waiters; i++)
    {
        if (task_waiter_threads[i])
        {
            pr_info("Softlockup Test: Stopping waiter thread %d.\n", i);
            kthread_stop(task_waiter_threads[i]);
            task_waiter_threads[i] = NULL;
        }
    }

    // Then stop holder thread
    if (task_holder_thread)
    {
        pr_info("Softlockup Test: Stopping holder thread.\n");
        kthread_stop(task_holder_thread);
        task_holder_thread = NULL;
    }

    pr_info("Softlockup Test: Module unloaded.\n");
}

module_init(softlockup_test_init);
module_exit(softlockup_test_exit);

MODULE_AUTHOR("Original: Saiyam Doshi, Modified by Assistant");
MODULE_DESCRIPTION("Test module to generate spinlock contention and trigger slowpath.");
MODULE_LICENSE("GPL v2");
