#!/usr/bin/env python3

import pandas as pd
import seaborn as sns
import matplotlib.pyplot as plt

import cpuinfo


l2_cache_size_bytes = cpuinfo.get_cpu_info()['l2_cache_line_size'] * 1000
values_per_l2 = l2_cache_size_bytes / 4

cpu_brand = cpuinfo.get_cpu_info()['brand_raw'].replace(' ', '').replace('@', '').replace('(', '').replace(')', '').replace('.', '_')

df = pd.read_csv('rel/results.csv')

for measurement in ['small', 'large', 'all']:
	df_filtered = df.copy()
	cutoff = values_per_l2 * 3.0
	if measurement == 'small':
		df_filtered = df.query('SIZE < @cutoff')
	elif measurement == 'large':
		df_filtered = df.query('SIZE > @cutoff')


	max_runtime = max(df_filtered.MEDIAN_RUNTIME_MUS)
	for thread_count in pd.unique(df.THREAD_COUNT):
		# first, l1 is for both measured CPUs 32kb. We cannot obtain the l1 cache from cpuinfo. 32kb is 8000 ints.
		df_filtered = df_filtered.append({'IMPLEMENTATION': 'L1' , 'THREAD_COUNT': thread_count, 'MEASUREMENTS': 1, 'SIZE': 8000, 'MEDIAN_RUNTIME_MUS': 0}, ignore_index=True)
		df_filtered = df_filtered.append({'IMPLEMENTATION': 'L1' , 'THREAD_COUNT': thread_count, 'MEASUREMENTS': 1, 'SIZE': 8000, 'MEDIAN_RUNTIME_MUS': max_runtime}, ignore_index=True)

		df_filtered = df_filtered.append({'IMPLEMENTATION': 'L2' , 'THREAD_COUNT': thread_count, 'MEASUREMENTS': 1, 'SIZE': values_per_l2, 'MEDIAN_RUNTIME_MUS': 0}, ignore_index=True)
		df_filtered = df_filtered.append({'IMPLEMENTATION': 'L2' , 'THREAD_COUNT': thread_count, 'MEASUREMENTS': 1, 'SIZE': values_per_l2, 'MEDIAN_RUNTIME_MUS': max_runtime}, ignore_index=True)

	g = sns.FacetGrid(df_filtered, col="THREAD_COUNT")
	g.map_dataframe(sns.lineplot, x="SIZE", y="MEDIAN_RUNTIME_MUS", hue="IMPLEMENTATION")
	g.set_axis_labels("Input size", "Runtime [microseconds]")
	g.add_legend()
	plt.savefig(f'{cpu_brand}__plot_{measurement}.pdf')

