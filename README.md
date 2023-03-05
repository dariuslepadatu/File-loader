Pentru aceasta tema am implementat un loader de fisiere executabile sub forma
unei bilioteci statice.

Initial, salvez file descriptorul intr-o variabila globala pentru a putea accesa fisierul
de oriunde. Se deschide fisierul pe care urmeaza sa il mapam in format read-only. In functia
segv_handler am construit handler-ul pentru SIGSEGV, fiind singurul semnal care ne intereseaza 
(atunci cand primim ca parametru un semnal diferit de SIGSEGV, se apeleaza handler-ul
default). Printr-un for se cauta segmentul care contine pagina la adresa careia a 
aparut page fault-ul. Folosesc campul void *data al structurii so_seg_t pentru a stoca
un vector de frecventa care contine datele despre paginile deja mapate. Daca pagina este
deja mapata, se apeleaza handler-ul default. Daca fault-ul este generat la o adresa care
se afla in segment, insa pagina nu este mapata, are loc "maparea efectiva": in vectorul data
se trece pagina respectiva drept mapata (seg_data[page] == 1) si se apeleaza functia mmap cu
flagurile MAP_PRIVATE, MAP_FIXED si MAP_ANONYMOUS. Aceste flaguri permit alocarea unei memorii
zeroizate. Daca marimea fisierului din segment (segment.file_size) depaseste marimea paginii
(page * page_size), mapam din nou, sau citim datele din fisier.

La final, folosim functia mprotect() pentru a seta permisiunile segmentului pe zona
de memorie mapata si dezalocam memoria alocata in vectorul data, dandu-i free in functia so_execute.

