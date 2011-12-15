#include <stdio.h>
#include <UserTrace.h>
#include <LinuxEvents.h>

struct delta_event
{
  int   an_int;
  char  a_char;
};

static int alpha_id, omega_id, theta_id, delta_id, rho_id;

int first_part(void)
{
  uint8_t a_byte;
  char trace_string[256];
  struct delta_event a_delta_event;

  /* Create events */
  alpha_id = trace_create_event("Alpha",
			     "Number %d, String %s, Hex %08X",
			     CUSTOM_EVENT_FORMAT_TYPE_STR,
			     NULL);
  omega_id = trace_create_event("Omega",
				"Number %d, Char %c",
				CUSTOM_EVENT_FORMAT_TYPE_STR,
				NULL);
  theta_id = trace_create_event("Theta",
				"Plain string",
				CUSTOM_EVENT_FORMAT_TYPE_STR,
				NULL);
  delta_id = trace_create_event("Delta",
				NULL,
				CUSTOM_EVENT_FORMAT_TYPE_HEX,
				NULL);
  rho_id = trace_create_event("Rho",
			      NULL,
			      CUSTOM_EVENT_FORMAT_TYPE_HEX,
			      NULL);

  /* Were the events created appropriately? */
  if((alpha_id < 0) ||
     (omega_id < 0) ||
     (theta_id < 0) ||
     (delta_id < 0) ||
     (rho_id < 0))
    {
    printf("Unable to allocate event IDs ... Exiting \n");
    exit(1);
    }

  /* Trace events */
  sprintf(trace_string, "String for Alpha ID: %d", alpha_id);
  trace_user_event(alpha_id, strlen(trace_string) + 1, trace_string);
  sprintf(trace_string, "String for Omega ID: %d", omega_id);
  trace_user_event(omega_id, strlen(trace_string) + 1, trace_string);
  sprintf(trace_string, "String for Theta ID: %d", theta_id);
  trace_user_event(theta_id, strlen(trace_string) + 1, trace_string);

  memset(&a_delta_event, 0, sizeof(a_delta_event));
  trace_user_event(delta_id, sizeof(a_delta_event), &a_delta_event);

  a_byte = 0x12;
  trace_user_event(rho_id, sizeof(a_byte), &a_byte);

  return 0;
}

void second_part(void)
{
  uint8_t a_byte;
  char trace_string[256];
  struct delta_event a_delta_event;

  /* Trace events */
  sprintf(trace_string, "Another String for Alpha ID: %d", alpha_id);
  trace_user_event(alpha_id, strlen(trace_string) + 1, trace_string);
  sprintf(trace_string, "Another String for Omega ID: %d", omega_id);
  trace_user_event(omega_id, strlen(trace_string) + 1, trace_string);
  sprintf(trace_string, "Another String for Theta ID: %d", theta_id);
  trace_user_event(theta_id, strlen(trace_string) + 1, trace_string);

  memset(&a_delta_event, 0xFF, sizeof(a_delta_event));
  trace_user_event(delta_id, sizeof(a_delta_event), &a_delta_event);
  a_byte = 0xA4;
  trace_user_event(rho_id, sizeof(a_byte), &a_byte);

  /* Destroy the events created */
  trace_destroy_event(alpha_id);
  trace_destroy_event(omega_id);
  trace_destroy_event(theta_id);
  trace_destroy_event(delta_id);
  trace_destroy_event(rho_id);
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
