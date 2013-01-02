import sys
import os

avg = lambda d: float(sum(d))/len(d)

for fname in os.listdir("."):
    if fname[:7] != "rfbench":
        continue
    
    print "..."
    f = open(fname, "r")
    lines = f.readlines()
    data = lines[9:-1]

    # Ignore some tests
    ignore = lines[6].split()
    warmup, cooldown = int(ignore[2]), int(ignore[6])
    if cooldown == 0:
        data = data[warmup:]
    else:
        data = data[warmup:-cooldown]

    # Get test data
    n_switches = int(data[3].split()[1])
    bench, controller, type_ = fname.split("_")

    # Get the final results
    min_, max_, avg_, stdev = [float(x) for x in lines[-1].split()[7].split("/")]

    # Process data
    values = []
    for d in data:
        columns = d.split()
        if len(columns) == 10:
            v = (int(columns[4]),)
        if len(columns) == 13:
            v = [int(x) for x in columns[4:8]]

        if type_ == "latency":
            values.append(1000 / avg(v))
        else:
            values.append(avg(v))

    # Output data
    ofile = open("data_%s_avg.csv" % fname, "w")

    m = len(values)
    insts = []
    for value in set(values):
        insts.append((value, values.count(value)))
    insts.sort(key=lambda x: x[0])

    c = 0.0
    for value, freq in insts:
        c += freq
        ofile.write("%f,%f\n" % (value, c/m))

    ofile.close()

## Output plot instructions
#ofile = open("plot", "w")

#plot = """set datafile separator ','

#set title "Latency for %i switch(es) - Accumulated"
#set key right bottom
#set xlabel "Latency (ms)" font "Helvetica,32"
#set ylabel "Cumulative Fraction" font "Helvetica,32"

#set pointsize 3
#set key right bottom

#set yrange [0:1]

#set term png
#set output "graph.png"
#plot %s
#"""



#s = ""
#for file_ in files:
#    print file_
#    s += "'%s' using 1:2 title '%s' with linespoints lt 0 pt 2," % (file_, files[file_])
#s = s.rstrip(", ")

#plot = plot % (n_switches, s)
#ofile.write(plot)
#ofile.close()

## Plot
#call(["gnuplot", "plot"])
