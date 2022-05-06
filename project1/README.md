Onoma: Nikolaos Pnevmatikos
AM: 1115201900157

-------------------------------------------------------------------------------------------------

Ektelesh: make compile
./sniffer
h
./sniffer -p [ path_name ]

Ektelesh bash script:
chmod +x finder.sh
./finder.sh [ number of tld ]

-------------------------------------------------------------------------------------------------

Ypo8etoume oti o fakelos pros parakolou8ish einai arxika kenos h ta arxeia pou periexei den mas endiaferoun.

H main apotelei thn ontothta manager.
H oura ylopoih8hke me vector o opoios krataei to id tou ka8e worker kai mia bool metavliti gia to an einai dia8esimos.

Kata thn ektelesh tou programmatos dimiourgountai, an den yparxoun hdh oi fakeloi outfolder kai fifos, pou 8a periexoun ta .out arxeia kai named pipes antistoixa. Meta dimiourgeitai to pipe meta3u manager kai listener kai ginetai fork gia na dimiourgei8ei o listener. O listener anakateu8inei thn e3odo tou sto pipe kai ektelei thn inotifywait me orismata create kai moved_to, wste na parakolou8ei mono ta arxeia pou dimiourgountai h metakinountai ston fakelo pou dwsame ws path.

O manager diavazei thn e3odo pou exei gra4ei o listener sto pipe kai gramma gramma 8a e3agei to onoma tou arxeiou.
Auto to kanei diavazontas 7 + (mege8os path) grammata. Ekei kanei elegxo gia na dei an h entolh pou epestre4e h inotifywait einai h create h h move_to. An o xarakthras einai 'E' tote einai h Create kai ara to onoma tou arxeiou 8a arxizei 2 xarakthres meta, an oxi einai h move_to kai to onoma tou arxeiou arxizei 4 xarakthres meta. (px. urls/CREATE one).
Afou vre8ei o ari8mos ths epanali4hs pou 8a arxizei to onoma tou arxeiou, xarakthra xarakthra to arxeio apo8ikeuetai se ena string mexri na sinanti8ei "\n". 

Afou vre8ei to onoma tou arxeiou o manager diatrexei ton vector gia na vrei an yparxei worker kai an einai dia8esimos. An vre8ei worker tote den 8a ginei kainourio fork kai o manager 8a sunexisei thn diergasia pou vrike. An den vre8ei tote ginetai fork kai o worker pou dimiourgeitai mpainei sto vector.

o manager dimiourgei fifo gia ton ka8e worker me onoma "fifos/myfifo{process_id}". Anoigei to fifo me dikaiwmata write kai grafei to onoma tou arxeiou. Meta kanei reset to string pou eixe to onoma tou arxeiou, tis epanali4eis kai thn 8esi pou arxizei to string.

o ka8e worker ektelei thn synarthsh work() opou ekei anoigei to fifo me read dikaiwmata kai e3agei to onoma tou arxeiou gramma gramma, to anoigei kai dimiourgei kai to antistoixo .out arxeio. Diavazei gramma gramma ta periexomena tou arxeioy kai se ka8e keno h allagh grammhs elegxei an h le3h pou prokiptei einai url. An nai tote e3agei to location tou kai koitaei an to exei 3anadei gia na au3isei ton counter ton emfanisewn tou. Telos grafei sto .out arxeio to location, 2 kena ,ton ari8mo emfanishs kai mia allagh grammhs gia ka8e location. Otan teleiwsei mpainei se katastash stopped.

Otan erxetai ston manager SIGCHLD, dhladh oti kapoios worker stamatise h termatise, ekteleite h signal handling function child_available opou me thn waitpid pairnei to id tou worker pou einai stamatimenos kai vazei thn metavlith pou einai dia8esimos se true. Gia na ginei auto eprepe o vector pou krataei tous workers kai thn katastash tous na einai global.

Otan erxetai ston manager SIGINT dhladh otan o xrhsths pataei control-c ekteleitai h signal handling function terminate opou kanei thn global metavlith endprog se true. Auto simainei oti to infinate loop tou manager stamataei. Etsi o manager termatizei thn diergasia tou listener kai diagrafei ta named pipes meta3y ton workers. Telos stelnei SIGCONT se workers pou htan se katastash stopped(gia na mporoun na lavoun SIGINT sima) kai meta SIGINT sima se olous tous workers.

Otan erxetai SIGINT se enan worker ekteleitai h signal handling function worker_term opou kanei thn global metablhth endwork se true. auto simainei oti to infinate loop tou worker 8a teleiwsei kai o worker 8a termatisei. Prwta omws 8a teleiwsei tyxon doyleia pou exei na kanei se ena .out arxeio kai meta 8a termatisei.


-------------------------------------------------------------------------------------------------

Bash script:

Sto bash script meso ths find pairnw ola ta arxeio pou teleiwnoun se .out apo ton fakelo outfiles. Meta meso for, gia ka8e argument pou do8ike trexw grammi grammi to ka8e arxeio kai 4axnw an to .tld einai substring thw grammis. An nai au3anw ton metrith. telos gia ka8e tld ektipwnw poses fores emfanistike sta arxeia pou e3etastikan. 
