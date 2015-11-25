from netCDF4 import Dataset
import matplotlib.pyplot as plt
import numpy as np
from mpl_toolkits.basemap import Basemap
import sys


#import relevant variables
nc_filename = sys.argv[1]
nc_split = nc_filename.split(".")
nc_out = nc_split[0]+"_plot.png"
nc=Dataset(nc_filename,mode='r')

lon=nc.variables['lon'][:]
lat=nc.variables['lat'][:]
dens=nc.variables['dens'][:]

nc.close()

lat_ticks=np.arange(-90,90,30)
lon_ticks=np.arange(0,360,30)

#create map
plt.figure(figsize=(24,12))
m = Basemap(llcrnrlat=-90.,urcrnrlat=90.,
            llcrnrlon=0.,urcrnrlon=359.,
            resolution='c')
         
m.drawcoastlines()
m.drawmapboundary()
m.drawparallels(lat_ticks,labels=[0,0,0,1])
m.drawmeridians(lon_ticks,labels=[0,0,0,1])

x,y = m(*np.meshgrid(lon,lat))
#draw filled contours
pv=m.contourf(x,y,dens,levels=np.arange(0,0.3,.025),extend='both')
#add colorbar
cb=plt.colorbar(pv)
plt.title("%s density for %s %s"%(sys.argv[4],sys.argv[2],sys.argv[3]))
plt.savefig(nc_out,bbox_inches='tight')
