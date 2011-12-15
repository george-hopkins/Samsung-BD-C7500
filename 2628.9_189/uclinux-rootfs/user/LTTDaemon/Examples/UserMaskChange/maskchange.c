
#include <UserTrace.h>
#include <LinuxEvents.h>

int main(void)
{
  trace_event_mask    lEventMask;

  printf("Attaching to trace device \n");

  if(trace_attach() < 0)
    {
    printf("Unable to attach to trace device \n");
    exit(1);
    }  

  printf("Setting trace mask to 0x0 \n");

  lEventMask = 0;
  trace_set_event_mask(lEventMask);

  /* Wait for a while */
  sleep(10);

  printf("Enabling events one by one \n");

  trace_enable_event_trace(TRACE_IRQ_ENTRY);
  sleep(2);
  trace_enable_event_trace(TRACE_TRAP_ENTRY);
  sleep(2);
  trace_enable_event_trace(TRACE_KERNEL_TIMER);
  sleep(2);
  trace_enable_event_trace(TRACE_PROCESS);
  sleep(2);
  trace_enable_event_trace(TRACE_SOFT_IRQ);
  sleep(2);
  
  /* Check for some events */
  if(trace_is_event_traced(TRACE_IRQ_ENTRY))
    printf("Tracing: TRACE_IRQ_ENTRY \n");
  else
    printf("NOT Tracing: TRACE_IRQ_ENTRY \n");
  if(trace_is_event_traced(TRACE_IRQ_EXIT))
    printf("Tracing: TRACE_IRQ_EXIT \n");
  else
    printf("NOT Tracing: TRACE_IRQ_EXIT \n");
  if(trace_is_event_traced(TRACE_PROCESS))
    printf("Tracing: TRACE_PROCESS \n");
  else
    printf("NOT Tracing: TRACE_PROCESS \n");
  if(trace_is_event_traced(TRACE_FILE_SYSTEM))
    printf("Tracing: TRACE_FILE_SYSTEM \n");
  else
    printf("NOT Tracing: TRACE_FILE_SYSTEM \n");
  if(trace_is_event_traced(TRACE_CHANGE_MASK))
    printf("Tracing: TRACE_CHANGE_MASK \n");
  else
    printf("NOT Tracing: TRACE_CHANGE_MASK \n");

  /* Get the trace mask */
  trace_get_event_mask(&lEventMask);
  printf("Event mask is : %lX \n", lEventMask);

  printf("Detaching from trace device \n");

  if(trace_detach() < 0)
    {
    printf("Unable to detach to trace device \n");
    exit(1);
    }  

  printf("Done \n");

  /* We're out */
  exit(0);

  return 0;
}
