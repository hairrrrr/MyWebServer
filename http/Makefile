bin=httpserver
cgi_dir=cgi
cc=g++
LD_FLAGS=-std=c++17 -lpthread 
src=main.cc
webroot=wwwroot

.PHONY:ALL 
ALL:$(bin) CGI

$(bin):$(src)
	$(cc) $^ -o $@ $(LD_FLAGS)

CGI: 
	cd $(cgi_dir);make;cd -


.PHONY:clean
clean:
	rm -f $(bin)
	rm -rf output 
	cd $(cgi_dir);\
	make clean;\
	cd -

.PHONY: output 
output:
	mkdir output 
	cp $(bin) output 
	cp -r $(webroot) output 
	find $(cgi_dir) -type f -executable -exec cp {} output/$(webroot) \;