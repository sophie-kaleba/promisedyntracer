library(turbotracer)

dyntrace_promises({ f<-function() print("Hello")
f()
for(i in (1:10)){f<<-function() print("Hello")}}, "/home/grophi/Documents/Stage/promisedyntracer/output")
