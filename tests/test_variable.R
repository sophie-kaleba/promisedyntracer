library(refactoredtracer)
#library(rlang)

expr = quote({
library(refactoredtracer)
library(gstat)
library(refactoredtracer)
library(gstat)
x <- 5
#env_binding_unlock(baseenv())
assign("x", function(a,b) return(a-b), baseenv())
assign("x", function(a,b) return(a-b), baseenv())
assign("x", 15, baseenv())
assign("x", 20, baseenv())
f <- function() print("Modified")
f <<- function() print("Modified")
f()
#x <<- 1
#base::assign("mean", function(a,b) return(a-b), baseenv())
#assign("mean", function(a,b) return(a-b), baseenv())
#assign("mean", function(a,b) return(a-b), baseenv())
#assign("mean", function(a,b) return(a-b), baseenv())
#assign("mean", function(a,b) return(a-b), baseenv())
#base::with(baseenv(), mean(2,3))
#base::with(baseenv(), mean(2,3))
#with(baseenv(), mean(6,1))
#with(baseenv(), mean(4,1))
#with(baseenv(), pi)

})

#modify_mean_binding_assign <- function() {assign("mean", function(a,b) return(a-b), baseenv()) }
#modify_mean_binding_assign()
#modify_mean_binding_assign()
#mean(10,3)})

dyntrace_promises(eval(expr), "/home/diens/Documents/tools/R_exp/refactoredtracer/output")
