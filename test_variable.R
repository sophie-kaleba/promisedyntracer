library(turbotracer)
library(rlang)

expr = quote({
env_binding_unlock(baseenv())
#assign("mean", 5, baseenv())
base::assign("mean", function(a,b) return(a-b), baseenv())
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

dyntrace_promises(eval(expr), "/home/diens/Documents/tools/R_exp/turbotracer/output")
