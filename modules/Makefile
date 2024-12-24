# obj-m += 202000173_module_capture_memory_snapshot.o
# obj-m += 202000173_module_get_io_throttle.o
# obj-m += 202000173_module_statistics.o
obj-m += 202000173_module_tamalloc.o

all:
	make -C /home/christian/linux-6.5 M=$(PWD) modules
clean:
	make -C /home/christian/linux-6.5 M=$(PWD) clean