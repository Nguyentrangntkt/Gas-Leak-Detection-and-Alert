# Host tests compile real firmware units. UART uses host register storage;
# app/LCD tests mock peripherals and time. No physical timing verification.
ifeq ($(origin CC),default)
CC := py -3 -m ziglang cc
endif
PRODUCER := ../c8t6_producer
CONSUMER := ../ccu6_consumer
OUT := out
CFLAGS := -std=c11 -O1 -g -Wall -Wextra -Werror
CPPFLAGS := -I.
SUITES := mq2 protocol alarm_fsm ring debounce buzzer_config producer consumer uart_f103 uart_f401 lcd i2c_bus watchdog
TESTS := $(addprefix $(OUT)/test_,$(SUITES))
all: run
# Isolated objects per config; -MT keeps Zig .d targets on actual object paths.
define suite
$(OUT)/$(1):
	mkdir -p $$@
$(OUT)/$(1)/%.o: %.c | $(OUT)/$(1)
	$$(CC) $$(CPPFLAGS) $(4) -I$(2)/Inc $$(CFLAGS) -MMD -MP -MF"$$(@:.o=.d)" -MT"$$@" -c $$< -o $$@
$(OUT)/$(1)/%.o: $(2)/Src/%.c | $(OUT)/$(1)
	$$(CC) $$(CPPFLAGS) $(4) -I$(2)/Inc $$(CFLAGS) -MMD -MP -MF"$$(@:.o=.d)" -MT"$$@" -c $$< -o $$@
$(OUT)/test_$(1): $(addprefix $(OUT)/$(1)/,$(addsuffix .o,$(3)))
	$$(CC) $$^ -o $$@ -lm
endef
$(eval $(call suite,mq2,$(PRODUCER),test_mq2 mq2_sensor))
$(eval $(call suite,protocol,$(PRODUCER),test_protocol protocol))
$(eval $(call suite,alarm_fsm,$(CONSUMER),test_alarm_fsm alarm_fsm protocol))
$(eval $(call suite,ring,$(PRODUCER),test_ring))
$(eval $(call suite,debounce,$(CONSUMER),test_debounce debounce))
$(eval $(call suite,buzzer_config,$(CONSUMER),test_buzzer_config alarm_fsm,-DCFG_BUZZER_FROM_LEVEL=3))
$(eval $(call suite,producer,$(PRODUCER),test_producer app_producer mq2_sensor protocol))
$(eval $(call suite,consumer,$(CONSUMER),test_consumer app_consumer alarm_fsm protocol))
$(eval $(call suite,uart_f103,$(PRODUCER),test_uart_driver drv_uart protocol,-Istubs -DTEST_UART_PRODUCER))
$(eval $(call suite,uart_f401,$(CONSUMER),test_uart_driver drv_uart protocol,-Istubs))
$(eval $(call suite,lcd,$(PRODUCER),test_lcd drv_lcd1602))
$(eval $(call suite,i2c_bus,$(PRODUCER),test_i2c_bus i2c_bus_clear))
$(eval $(call suite,watchdog,$(CONSUMER),test_watchdog drv_iwdg,-Istubs))
compile: $(TESTS)
run: $(TESTS)
	@fail=0; for t in $(TESTS); do ./$$t || fail=1; done; \
	 if [ $$fail -eq 0 ]; then echo "TAT CA BO TEST DEU DAT"; \
	 else echo "CO BO TEST THAT BAI"; exit 1; fi
clean:
	rm -rf $(OUT)
.PHONY: all compile run clean
-include $(wildcard $(OUT)/*/*.d)
