gnuplot << EOF
	set terminal png size 1920,1080
	set output "graph_cpu.png"
	set title "Temps de génération"
	set xlabel "Nombre de coeur"
	set ylabel "Temps (en s)"

	plot "cpu_graph.plot" u 1:2 title "normal" w l, \
	"cpu_graph.plot" u 1:2:3 title "interval de confiance normal" w yerrorbars, \
	"cpu_graph.plot" u 1:4 title "MPI" w l , \
	"cpu_graph.plot" u 1:4:5 title "interval de confiance MPI" w yerrorbars
EOF
