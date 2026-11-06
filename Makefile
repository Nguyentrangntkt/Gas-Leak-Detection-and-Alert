# Build va kiem thu toan bo du an tu dong lenh.
#
# Day la duong build phu tro de kiem chung nhanh; STM32CubeIDE van build
# hai project bang .cproject nhu binh thuong (xem README.md).
#
#   make            -> kiem ban sao + unit test tren PC + build ca hai firmware
#   make check-sync -> kiem bon file dung chung giua hai project
#   make test       -> chi unit test
#   make firmware   -> chi build hai firmware STM32
#   make clean
#
# Neu arm-none-eabi-gcc khong o PATH, chi duong dan toolchain:
#   make ARM_BIN="/c/ST/STM32CubeIDE_2.2.0/STM32CubeIDE/plugins/com.st.stm32cube.ide.mcu.externaltools.gnu-tools-for-stm32.14.3.rel1.win32_1.0.100.202602081740/tools/bin/"

ARM_BIN ?=

all: check-sync test firmware

# Bon file duoi day ton tai hai ban giong het nhau trong hai project, vi
# ca hai firmware deu can chung ma khong co thu muc dung chung nao trong
# cau truc project cua STM32CubeIDE. Truoc day khong co gi EP chung phai
# khop: sua protocol.c mot ben roi quen ben kia la hai thiet bi noi hai
# phuong ngu khac nhau, va trieu chung se la "khung hop le bi tu choi" -
# rat kho truy. Chot nay bien loi do thanh loi build.
#
# -I bo qua dong tu mo ta ban sao (moi ban tro ve ban kia), nen chi khac
# biet that su o phan ma moi lam build do.
# syscalls.c/sysmem.c la scaffolding RIENG do CubeIDE sinh cho moi target.
# Hien giong nhau nhung khong phai giao keo dung chung; IDE co the tai sinh
# khac nhau hop le. Co y KHONG ep chung dong bo va khong sua ma vendor.
SYNC_FILES := Src/protocol.c Inc/protocol.h Inc/ring.h Inc/uart_rx_event.h

check-sync:
	@fail=0; for f in $(SYNC_FILES); do \
	  if ! diff -I 'BAN SAO DONG BO' "c8t6_producer/$$f" "ccu6_consumer/$$f" >/dev/null; then \
	    echo "LECH BAN SAO: $$f"; \
	    diff -I 'BAN SAO DONG BO' "c8t6_producer/$$f" "ccu6_consumer/$$f" || true; \
	    fail=1; \
	  fi; \
	done; \
	if [ $$fail -ne 0 ]; then echo "" ; echo "Hai ban sao da lech nhau - dong bo lai truoc khi build."; exit 1; fi; \
	echo "ban sao giua hai project: khop"

test:
	$(MAKE) -C test_host run

firmware: producer consumer

producer:
	$(MAKE) -C c8t6_producer TOOLCHAIN="$(ARM_BIN)"

consumer:
	$(MAKE) -C ccu6_consumer TOOLCHAIN="$(ARM_BIN)"

clean:
	$(MAKE) -C test_host clean
	$(MAKE) -C c8t6_producer clean
	$(MAKE) -C ccu6_consumer clean

.PHONY: all check-sync test firmware producer consumer clean
