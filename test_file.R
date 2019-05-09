library(turbotracer)

dyntrace_promises({ ###############################################################################
# metric kriging for different spatio-temporal anisotropy rates and different #
# "scales": residuals, log-scale, direct kriging                              #
###############################################################################
install.packages("gstat", repos="http://cloud.r-project.org/")
library(gstat)

load("EU_RB_2009_rev.RData") # from MultiReg_2009.R if necessary

# define spatio-temporal anisotropy
tmpScale <- 164e3

## set-up a metric 3D-model
locs <- EU_RB_2009@sp

# use for direct metric kriging
# air3d <- cbind(EU_RB_2009@data$PM10, rep(locs@coords[,1],365), rep(locs@coords[,2],365), 
#                rep((1:365),each=length(EU_RB_2009@sp))*tmpScale)

# use for log resiudal kriging
air3d <- cbind(EU_RB_2009@data$resid, rep(locs@coords[,1],365), rep(locs@coords[,2],365), 
               rep((1:365),each=length(EU_RB_2009@sp))*tmpScale)

# use for direct kriging on log-scale:
# air3d <- cbind(log(EU_RB_2009@data$PM10), rep(locs@coords[,1],365), rep(locs@coords[,2],365), 
#                rep((1:365),each=length(EU_RB_2009@sp))*tmpScale)

colnames(air3d) <- c("PM10","x","y","t")
air3d <- as.data.frame(air3d)

air3d$PM10[is.infinite(air3d$PM10)] <- NA

coordinates(air3d) <- ~x+y+t
proj4string(air3d) <- CRS()

# fit the kriging model
gstat3d <- gstat(formula=PM10~1, data=air3d[!is.na(air3d$PM10),])
vgm3d <- variogram(gstat3d, 1e6)

model3d <- fit.variogram(vgm3d, model=vgm(.4, "Sph", 1000e3, .2))
plot(vgm3d, model3d)

###
# do cross validation
###
nmax <- 100 # maximum of involved neighbours

k3D_x_estims <- NULL
k3D_x_var <- NULL
for (i in 1:length(EU_RB_2009@sp)) {
cat("Location:",i,"\n")
  tmp_pred <- data.frame(cbind(rep(EU_RB_2009@sp@coords[i,1],365),
                               rep(EU_RB_2009@sp@coords[i,2],365),
                               (1:365)*tmpScale))
  colnames(tmp_pred) <- c("x","y","t")
  coordinates(tmp_pred) <- ~x+y+t
  tmp_air3d <- air3d[-((0:364)*length(EU_RB_2009@sp)+i),]
  tmp_krige <- krige(formula=PM10~1, locations=tmp_air3d[as.vector(!is.na(tmp_air3d@data)),],  
                     newdata=tmp_pred, model3d, nmax=nmax)
  k3D_x_estims <- rbind(k3D_x_estims, as.vector(tmp_krige@data[,1]))
  k3D_x_var <- rbind(k3D_x_var, as.vector(tmp_krige@data[,2]))
}  

# use for direct metric kriging, but mind the air3d and model3d objects
# k3D_x_pred <- k3D_x_estims

# use for log resiudal kriging, but mind the air3d and model3d objects
k3D_x_pred <- exp(log(EU_RB_2009$pred) + k3D_x_estims + k3D_x_var/2) 

# use for direct kriging on log-scale, but mind the air3d and model3d objects
# k3D_x_pred <- exp(k3D_x_estims - k3D_x_var/2) 

EU_RB_2009$k3DXpred <- as.vector(k3D_x_pred)

k3D_x_yearly <- apply(X=k3D_x_pred, MARGIN=1, FUN=mean, na.rm=T)
EU_RB_2009_mean$k3DXpred <- k3D_x_yearly

save(vgm3d, model3d, k3D_x_estims, k3D_x_var, k3D_x_pred, k3D_x_yearly, 
     file=paste("k3D_x_2009_n100_",tmpScale/1000,".Rdata",sep=""))
save(EU_RB_2009, EU_RB_2009_mean, coef, file="EU_RB_2009_rev.RData")

k3D_daily_RMSE <- sqrt(mean((EU_RB_2009$k3DXpred - EU_RB_2009$PM10)^2,na.rm=T))
k3D_daily_BIAS <- -mean(EU_RB_2009$k3DXpred - EU_RB_2009$PM10,na.rm=T)
k3D_daily_MAE <- mean(abs(EU_RB_2009$k3DXpred - EU_RB_2009$PM10),na.rm=T)
k3D_daily_COR <- cor(EU_RB_2009$k3DXpred,EU_RB_2009$PM10, 
                     use="pairwise.complete.obs")

plot(EU_RB_2009_mean$k3DXpred ~ EU_RB_2009_mean$PM10, 
     main="yearly mean log(PM10) ~ model + alt + time; 3D res. kriging", 
     xlab="mean of measurements",ylab="mean of predictions",asp=1)
abline(a=0,b=1,col="red")

k3D_yearly_RMSE <- sqrt(mean((EU_RB_2009_mean$k3DXpred - EU_RB_2009_mean$PM10)^2,na.rm=T)) # 
k3D_yearly_BIAS <- -mean(EU_RB_2009_mean$k3DXpred - EU_RB_2009_mean$PM10,na.rm=T) # 
k3D_yearly_MAE <- mean(abs(EU_RB_2009_mean$k3DXpred - EU_RB_2009_mean$PM10),na.rm=T) # 
k3D_yearly_COR <- cor(EU_RB_2009_mean$k3DXpred, EU_RB_2009_mean$PM10,use="pairwise.complete.obs") # 

cat(paste(k3D_daily_RMSE, k3D_daily_BIAS, k3D_daily_MAE, k3D_daily_COR, 
          k3D_yearly_RMSE, k3D_yearly_BIAS, k3D_yearly_MAE, k3D_yearly_COR, sep="\n"))

save(k3D_daily_RMSE, k3D_daily_BIAS, k3D_daily_MAE, k3D_daily_COR, 
     k3D_yearly_RMSE, k3D_yearly_BIAS, k3D_yearly_MAE, k3D_yearly_COR, 
     file=paste("eval_k3D_x_2009_n100_",tmpScale/1000,".Rdata",sep=""))}, "/home/diens/Documents/tools/R_exp/turbotracer/output")
