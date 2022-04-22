#On ouvre les données
data = read.csv2('../res.csv', dec=".")


## --------large.mnt-------- ##
l_data = subset(data, data$file == "large.mnt")
l_time_moyen = aggregate(
          x= l_data$time,
          by = list(l_data$nbThreads),      
          FUN = mean)
nbProc = l_time_moyen$Group.1
l_temps = l_time_moyen$x
l_speed_up = l_temps[1]/l_temps
l_effi = l_speed_up/nbProc

## --------medium.mnt-------- ##
m_data = subset(data, data$file == "medium.mnt")
m_time_moyen = aggregate(
  x= m_data$time,
  by = list(m_data$nbThreads),      
  FUN = mean)
m_temps = m_time_moyen$x
m_speed_up = m_temps[1]/m_temps
m_effi = m_speed_up/nbProc

## --------small.mnt-------- ##
s_data = subset(data, data$file == "small.mnt")
s_time_moyen = aggregate(
  x= s_data$time,
  by = list(s_data$nbThreads),      
  FUN = mean)
s_temps = s_time_moyen$x
s_speed_up = s_temps[1]/s_temps
s_effi = s_speed_up/nbProc

## --------mini.mnt-------- ##
mi_data = subset(data, data$file == "mini.mnt")
mi_time_moyen = aggregate(
  x= mi_data$time,
  by = list(mi_data$nbThreads),      
  FUN = mean)
mi_temps = mi_time_moyen$x
mi_speed_up = mi_temps[1]/mi_temps
mi_effi = mi_speed_up/nbProc


## --------TEMPS-------- ##
plot(
  main="tmpExec/nbThreads",
  xlab="nbThreads",
  ylab="tmpExec",
  xlim=c(min(nbProc),max(nbProc)), 
  ylim=c(0,max(c(l_temps, m_temps, s_temps, mi_temps))), 
  x= nbProc,
  y= l_temps,
  type='o',
  col="blue"
)
points(
  x= nbProc,
  y= m_temps,
  type="o",
  col="red"
)
points(
  x= nbProc,
  y= s_temps,
  type="o",
  col="green"
)
points(
  x= nbProc,
  y= mi_temps,
  type="o",
  col="skyblue"
)
legend("topright", 
       legend = c("Large", "Medium", "Small", "Mini"), 
       col = c("blue","red", "green", "skyblue"), 
       lty=1)

## --------SPEEDUP-------- ##
plot(
  main="speedUp/nbThreads",
  xlab="nbThreads",
  ylab="speedup",
  xlim=c(min(nbProc),max(nbProc)), 
  ylim=c(0,max(c(l_speed_up, m_speed_up, s_speed_up,mi_speed_up))), 
  x= nbProc,
  y= l_speed_up,
  type='o',
  col="blue"
)
points(
  x= nbProc,
  y= m_speed_up,
  type="o",
  col="red"
)
points(
  x= nbProc,
  y= s_speed_up,
  type="o",
  col="green"
)
points(
  x= nbProc,
  y= mi_speed_up,
  type="o",
  col="skyblue"
)
legend("topleft", 
       legend = c("Large", "Medium", "Small", "Mini"), 
       col = c("blue","red", "green", "skyblue"), 
       lty=1)


# ## --------EFFICIENCY-------- ##
plot(
  main="efficiency/nbThreads",
  xlab="nbThreads",
  ylab="efficiency",
  xlim=c(min(nbProc),max(nbProc)), 
  ylim=c(0,max(c(l_effi, m_effi, s_effi, mi_effi))), 
  x= nbProc,
  y= l_effi,
  type='o',
  col="blue"
)
points(
  x= nbProc,
  y= m_effi,
  type="o",
  col="red"
)
points(
  x= nbProc,
  y= s_effi,
  type="o",
  col="green"
)
points(
  x= nbProc,
  y= mi_effi,
  type="o",
  col="skyblue"
)
legend("topright", 
       legend = c("Large", "Medium", "Small", "Mini"), 
       col = c("blue","red", "green", "skyblue"), 
       lty=1)

