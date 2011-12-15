#if 0
#define CONFIG_LTT
#endif

#include <linux/init.h>
#include <linux/config.h>
#include <linux/module.h>
#include <linux/ltt-core.h>
#include <asm/string.h>

struct delta_event
{
  int   an_int;
  char  a_char;
};

static int alpha_id, omega_id, theta_id, delta_id, rho_id;

int custom_init(void)
{
  uint8_t a_byte;
  char a_char;
  int an_int;
  int a_hex;
  char* a_string = "We are initializing the module";
  struct delta_event a_delta_event;

  /* Create events */
  alpha_id = ltt_create_event("Alpha",
			      "Number %d, String %s, Hex %08X",
			      CUSTOM_EVENT_FORMAT_TYPE_STR,
			      NULL);
  omega_id = ltt_create_event("Omega",
			      "Number %d, Char %c",
			      CUSTOM_EVENT_FORMAT_TYPE_STR,
			      NULL);
  theta_id = ltt_create_event("Theta",
			      "Plain string",
			      CUSTOM_EVENT_FORMAT_TYPE_STR,
			      NULL);
  delta_id = ltt_create_event("Delta",
			      NULL,
			      CUSTOM_EVENT_FORMAT_TYPE_HEX,
			      NULL);
  rho_id = ltt_create_event("Rho",
			    NULL,
			    CUSTOM_EVENT_FORMAT_TYPE_HEX,
			    NULL);

  /* Trace events */
  an_int = 1;
  a_hex = 0xFFFFAAAA;
  ltt_log_std_formatted_event(alpha_id, an_int, a_string, a_hex);
  an_int = 25;
  a_char = 'c';
  ltt_log_std_formatted_event(omega_id, an_int, a_char);
  ltt_log_std_formatted_event(theta_id);
  memset(&a_delta_event, 0, sizeof(a_delta_event));
  ltt_log_raw_event(delta_id, sizeof(a_delta_event), &a_delta_event);
  a_byte = 0x12;
  ltt_log_raw_event(rho_id, sizeof(a_byte), &a_byte);

  return 0;
}

void custom_exit(void)
{
  uint8_t a_byte;
  char a_char;
  int an_int;
  int a_hex;
  char* a_string = "We are cleaning up the module";
  struct delta_event a_delta_event;

  /* Trace events */
  an_int = 324;
  a_hex = 0xABCDEF10;
  ltt_log_std_formatted_event(alpha_id, an_int, a_string, a_hex);
  an_int = 789;
  a_char = 's';
  ltt_log_std_formatted_event(omega_id, an_int, a_char);
  ltt_log_std_formatted_event(theta_id);
  memset(&a_delta_event, 0xFF, sizeof(a_delta_event));
  ltt_log_raw_event(delta_id, sizeof(a_delta_event), &a_delta_event);
  a_byte = 0xA4;
  ltt_log_raw_event(rho_id, sizeof(a_byte), &a_byte);

  /* Destroy the events created */
  ltt_destroy_event(alpha_id);
  ltt_destroy_event(omega_id);
  ltt_destroy_event(theta_id);
  ltt_destroy_event(delta_id);
  ltt_destroy_event(rho_id);
}

module_init(custom_init);
module_exit(custom_exit);

MODULE_LICENSE("GPL");
