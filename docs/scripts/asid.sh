
for j in {"traces/cxx.txt.gz traces/ls.txt.gz","traces/cxx.txt.gz traces/ls.txt.gz traces/make.txt.gz","traces/*.txt.gz"} ;
    do
    for i in {"-a",""} ; do 
        echo -n "$j, $i: " ; ./pagetables -t 64 $i -R $j 2>&1 | grep hit ; 
    done
done