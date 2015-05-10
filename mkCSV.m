
load('C:\Home\Project\Binaural\CIPIC_hrtf_database\standard_hrir_database\subject_008\hrir_final.mat')

format long;

dlmwrite('hrir_l.csv', hrir_l, 'delimiter', ',', 'precision', '%1.16f');

dlmwrite('hrir_r.csv', hrir_r, 'delimiter', ',', 'precision', '%1.16f');
dlmwrite('ITD.csv', IDT, 'delimiter', ',', 'precision', '%1.16f');
dlmwrite('OnL.csv', OnL, 'delimiter', ',', 'precision', '%1.16f');
dlmwrite('OnR.csv', OnR, 'delimiter', ',', 'precision', '%1.16f');
