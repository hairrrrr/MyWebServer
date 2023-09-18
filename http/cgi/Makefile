.PHONY:ALL
ALL:calculator_cgi show_resume

calculator_cgi:calculator.cc 
	g++ $^ -o  $@

show_resume:show_resume.cc
	g++ $^ -o $@

.PHONY:clean 
clean:
	rm calculator_cgi show_resume 