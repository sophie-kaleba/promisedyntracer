f<-function() print("Hello")
f()
for(i in (1:10)){f<<-function() print("Hello")}
