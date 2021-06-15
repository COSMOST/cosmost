import sys
import pandas as pd
import re
import csv
import matplotlib.pyplot as plt
import numpy as np

labels={
	'2214.csv':'2214, 1.4M',
	'2219.csv':'2219, 1.9M'
}

linestyles = ['-', ':', '--', '-.', 'solid', 'dashed', 'dotted']
colors = ['green', 'blue', 'red', 'black', 'olive', 'brown', 'magenta']
markers = ['^', '+', 'None', 'x', '<', '>', '8', 's', 'p', '*', 'h', 'H', 'D', 'd', 'P', 'X']

SMALL_SIZE = 15
MEDIUM_SIZE = 22
BIGGER_SIZE = 22

plt.rc('font', size=SMALL_SIZE)          # controls default text sizes
plt.rc('axes', titlesize=MEDIUM_SIZE)    # fontsize of the axes title
plt.rc('axes', labelsize=MEDIUM_SIZE)    # fontsize of the x and y labels
plt.rc('xtick', labelsize=SMALL_SIZE)    # fontsize of the tick labels
plt.rc('ytick', labelsize=SMALL_SIZE)    # fontsize of the tick labels
plt.rc('legend', fontsize=SMALL_SIZE)    # legend fontsize
#plt.rc('label', titlesize=BIGGER_SIZE)  # fontsize of the figure title

def plot_bars(ax, sizes2range, idx):
	# arrange
	size_labels = ['50MB','100MB','1GB','2GB','3GB','4GB']

	# get total apps for each size range
	total_apps = 0
	for size_label in size_labels:
		total_apps += sizes2range[size_label]

	# get percent apps for each size range
	data_list = []
	for size_label in size_labels:
		app_count = sizes2range[size_label]
		data_list.append(100*app_count/total_apps)

	# plot graph
	print(sizes2range)
	print(data_list)

	X = np.arange(len(size_labels))
	Y = data_list
	width = 0.4

	plt.bar(X+(idx*width), Y, align='center',
			edgecolor='black', color=colors, \
			width=width, alpha=0.5, zorder=11)
	plt.xticks(X, size_labels)

	#df = pd.DataFrame(sizes2range_dict, index=[0])
	#print(df)
	#df.plot(ax=ax, kind="bar", \
	#		edgecolor='black', color=colors, \
	#		width=0.8, fontsize=16, align='center', \
	#		alpha=0.5, rot=0, zorder=11, legend=False)

def load_data(filename):
	sizes2range = {}
	with open(filename, mode='r') as f:
		reader = csv.reader(f.readlines())
		next(reader)  #This skips the first row of the CSV file.
		for line in reader:
			size = int(line[0].strip())
			if size <= (50*1024*1024):
				size_label = '50MB'
			elif size <= (100*1024*1024):
				size_label = '100MB'
			elif size <= (1024*1024*1024):
				size_label = '1GB'
			elif size <= (2*1024*1024*1024):
				size_label = '2GB'
			elif size <= (3*1024*1024*1024):
				size_label = '3GB'
			elif size <= (4*1024*1024*1024):
				size_label = '4GB'
			if size_label not in sizes2range:
				sizes2range[size_label] = 1
			else:
				sizes2range[size_label] += 1
	return sizes2range

if __name__ == "__main__":

	# validate args
	if len(sys.argv) < 2:
		print("Usage: python3 %s <2214.csv> <2216.csv> <2219.csv>")
		exit(1)

	ax = plt.gca()
	ax.set_ylim([0,0.002])
	ax.tick_params(axis = 'both', which = 'major', labelsize = 22)

	sizes2range_dict = {'50MB':[],'100MB':[],'1GB':[],'2GB':[],'3GB':[],'4GB':[]}
	for idx in range(1, len(sys.argv)):
		filename = sys.argv[idx]
	
		# plot figure
		sizes2range = load_data(filename)
		plot_bars(ax, sizes2range, idx - 1)

		#for size_label, app_count in sizes2range.items():
		#	sizes2range_dict[size_label].append(app_count)
		#plot_bars(ax, sizes2range_dict)


	#plt.yscale('log')
	
	plt.grid(True)
	plt.xlabel("Size range", fontsize=22)
	plt.ylabel("# Apps", fontsize=22)
	
	# https://stackoverflow.com/questions/9834452/how-do-i-make-a-single-legend-for-many-subplots-with-matplotlib
	#handles, labels = ax[0].get_legend_handles_labels()
	# source: https://stackoverflow.com/questions/4700616/how-to-put-the-legend-out-of-the-plot
	#fig.legend(handles, labels, bbox_to_anchor=(0.0, 1.0), fontsize=30, ncol=5, loc='center left')
	#plt.legend(fontsize=22, ncol=2, loc='lower right')
	plt.tight_layout()
	plt.show()
	#plt.savefig(filename+'-downloads.svg')
