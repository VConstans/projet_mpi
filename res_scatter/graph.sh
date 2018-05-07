gnuplot << EOF
	set terminal png size 1920,1080
	set output "graph.png"
	set title "Temps de génération"
	set xlabel "Taille"
	set ylabel "Temps"

	plot "graph.plot" u 1:2 title "normal" w l, \
	"graph.plot" u 1:2:3 title "normal" w yerrorbars, \
	"graph.plot" u 1:4 title "MPI" w l , \
	"graph.plot" u 1:4:5 title "MPI" w yerrorbars
EOF
