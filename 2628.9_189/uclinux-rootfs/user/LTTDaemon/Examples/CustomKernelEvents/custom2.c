#if 0
#define CONFIG_TRACE
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

static int alpha_id, omega_id;

int custom_init(void)
{
  uint8_t a_byte;
  struct delta_event a_delta_event;

  /* Create events */
  alpha_id = ltt_create_event("Alpha",
			      NULL,
			      CUSTOM_EVENT_FORMAT_TYPE_XML,
			      "<event name=\"Alpha\" size=\"0\"><var name=\"an_int\" type=\"int\" /><var name=\"a_char\" type=\"char\" /></event>");
  omega_id = ltt_create_event("Omega",
			      NULL,
			      CUSTOM_EVENT_FORMAT_TYPE_XML,
			      "<event name=\"Omega\" size=\"0\"><var name=\"a_byte\" type=\"u8\" /></event>");

  /* Trace events */
  memset(&a_delta_event, 0, sizeof(a_delta_event));
  ltt_log_raw_event(alpha_id, sizeof(a_delta_event), &a_delta_event);
  a_byte = 0x12;
  ltt_log_raw_event(omega_id, sizeof(a_byte), &a_byte);
  return 0;
}

void custom_exit(void)
{
  uint8_t a_byte;
  struct delta_event a_delta_event;

  /* Trace events */
  memset(&a_delta_event, 0xFF, sizeof(a_delta_event));
  ltt_log_raw_event(alpha_id, sizeof(a_delta_event), &a_delta_event);
  a_byte = 0xA4;
  ltt_log_raw_event(omega_id, sizeof(a_byte), &a_byte);

  /* Destroy the events created */
  ltt_destroy_event(alpha_id);
  ltt_destroy_event(omega_id);
}

module_init(custom_init);
module_exit(custom_exit);

MODULE_LICENSE("GPL");
