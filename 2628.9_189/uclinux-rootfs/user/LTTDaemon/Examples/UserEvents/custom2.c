#include <stdio.h>
#include <UserTrace.h>
#include <LinuxEvents.h>

struct delta_event
{
  int   an_int;
  char  a_char;
};

static int alpha_id, omega_id;

int first_part(void)
{
  uint8_t a_byte;
  char a_char;
  int an_int;
  int a_hex;
  char* a_string = "We are initializing the module";
  struct delta_event a_delta_event;

  /* Create events */
  alpha_id = trace_create_event("Alpha",
				NULL,
				CUSTOM_EVENT_FORMAT_TYPE_XML,
				"<event name=\"Alpha\" size=\"0\"><var name=\"an_int\" type=\"int\" /><var name=\"a_char\" type=\"char\" /></event>");
  omega_id = trace_create_event("Omega",
			        NULL,
				CUSTOM_EVENT_FORMAT_TYPE_XML,
				"<event name=\"Omega\" size=\"0\"><var name=\"a_byte\" type=\"u8\" /></event>");

  /* Trace events */
  memset(&a_delta_event, 0, sizeof(a_delta_event));
  trace_user_event(alpha_id, sizeof(a_delta_event), &a_delta_event);
  a_byte = 0x12;
  trace_user_event(omega_id, sizeof(a_byte), &a_byte);
  return 0;
}

void second_part(void)
{
  uint8_t a_byte;
  char a_char;
  int an_int;
  int a_hex;
  char* a_string = "We are initializing the module";
  struct delta_event a_delta_event;

  /* Trace events */
  memset(&a_delta_event, 0xFF, sizeof(a_delta_event));
  trace_user_event(alpha_id, sizeof(a_delta_event), &a_delta_event);
  a_byte = 0xA4;
  trace_user_event(omega_id, sizeof(a_byte), &a_byte);

  /* Destroy the events created */
  trace_destroy_event(alpha_id);
  trace_destroy_event(omega_id);
}

int main(void)
{
  printf("Attaching to trace device \n");

  if(trace_attach() < 0)
    {
    printf("Unable to attach to trace device \n");
    exit(1);
    }  

  printf("Calling first part \n");

  /* Call first portion */
  first_part();

  printf("Calling second part \n");

  /* Call first portion */
  second_part();

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
