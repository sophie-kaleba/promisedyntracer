library(turbotracer)
library(readr)



# extract the R code of the file given as parameter, store it into an quoted expression and trace it using the turbotracer

dyntrace_promises(eval(parse(text=read_file("test_parse.R"))), "/home/diens/Documents/tools/R_exp/turbotracer/output")
