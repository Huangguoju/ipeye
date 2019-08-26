
#objects = main.o
#main.o : main.c
edit : 
	arm-hisiv100nptl-linux-gcc main.c  -lpthread -o IpEyeServer
	mv IpEyeServer lib_v100/
	
	arm-hisiv200-linux-gcc main.c  -lpthread -o IpEyeServer
	mv IpEyeServer lib_v200/
	
	arm-hisiv300-linux-gcc  main.c  -lpthread -o IpEyeServer
	mv IpEyeServer lib_v300/
	
	arm-hisiv300-linux-gcc  main.c  -lpthread -o IpEyeServer
	mv IpEyeServer lib_v300-A9/
	
	arm-histbv310-linux-gcc  main.c  -lpthread -o IpEyeServer
	mv IpEyeServer lib_v310/
	
	arm-histbv320-linux-gcc  main.c  -lpthread -o IpEyeServer
	mv IpEyeServer lib_v320/
	
	arm-hisiv400-linux-gcc  main.c  -lpthread -o IpEyeServer
	mv IpEyeServer lib_v400/
	
	arm-hisiv500-linux-gcc  main.c  -lpthread -o IpEyeServer
	mv IpEyeServer lib_v500/
	
	arm-hisiv500-linux-gcc  main.c  -lpthread -o IpEyeServer
	mv IpEyeServer lib_v500-A9/
	
	arm-hisiv510-linux-gcc  main.c  -lpthread -o IpEyeServer
	mv IpEyeServer lib_v510/
	
	arm-himix200-linux-gcc  main.c  -lpthread -o IpEyeServer
	mv IpEyeServer lib_x200-A53/
	
	arm-ca53-linux-gnueabihf-gcc  main.c  -lpthread -o IpEyeServer
	mv IpEyeServer lib_nt-ca53/

.PHONY: clean
clean:
	-rm -f lib_v100/IpEyeServer
	-rm -f lib_v200/IpEyeServer
	-rm -f lib_v300/IpEyeServer
	-rm -f lib_v300-A9/IpEyeServer
	-rm -f lib_v310/IpEyeServer
	-rm -f lib_v320/IpEyeServer
	-rm -f lib_v400/IpEyeServer
	-rm -f lib_v500/IpEyeServer
	-rm -f lib_v500-A9/IpEyeServer
	-rm -f lib_v510/IpEyeServer
	-rm -f lib_x200-A53/IpEyeServer
	
	
	