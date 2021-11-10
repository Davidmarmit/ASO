obj-m := fase1.o

TARGET_MODULE:=simple_module

ifneq ($(KERNELRELEASE),)
	$(TARGET_MODULE)-objs := fase1.o
	obj-m := $(TARGET_MODULE).o
else
	BUILDSYSTEM_DIR:=/lib/modules/$(shell uname -r)/build
	PWD:=$(shell pwd)
all:
	$(MAKE) -C $(BUILDSYSTEM_DIR) M=$(PWD) modules
clean:
	$(MAKE) -C $(BUILDSYSTEM_DIR) M=$(PWD) clean
install:
	insmod ./$(TARGET_MODULE).ko
uninstall:
	rmmod ./$(TARGET_MODULE).ko
endif
