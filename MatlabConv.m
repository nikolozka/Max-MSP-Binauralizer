load('C:\Home\Project\Binaural\CIPIC_hrtf_database\standard_hrir_database\subject_003\hrir_final.mat');

[x,Fs] = audioread('altj.wav');

az=6; %1:25
el=9; %1:50

az1=6;
az2=15;

azimuths = [-80 -65 -55 -45:5:45 55 65 80];
elevations = -45:5.625:230.625;

azimuth1 = azimuths(az1);
azimuth2 = azimuths(az2);

elevation = elevations(el);

delay1 = floor((ITD(az1,el)/1000)*Fs);
delay2 = floor((ITD(az2,el)/1000)*Fs);

Nb = 512;

x_l = x(:,1);
x_r = x(:,2);

l = length(x_l);

l_padded = ceil(l/Nb)*Nb;

padding = zeros(l_padded-l,1);

x_l_padded = vertcat(x_l, padding);
x_r_padded = vertcat(x_r, padding);

x_split_l = reshape(x_l_padded,[Nb,l_padded/Nb]);
x_split_r = reshape(x_r_padded,[Nb,l_padded/Nb]);


y = zeros(2, l_padded*2);

l_out = Nb+200-1;

for i=1:l_padded/Nb
    
  l_buff = x_split_l(:,i).';
  r_buff = x_split_r(:,i).';

  index_l_l = (i-1)*Nb+501;
  index_r_l = (i-1)*Nb+501;
  index_l_r = (i-1)*Nb+501;
  index_r_r = (i-1)*Nb+501;
  
  if(azimuth1>0 || elevation>0) 
    index_l_l=index_l_l+delay1;

  else
    index_r_l=index_r_l+delay1;
  end
  
  if(azimuth2>0 || elevation>0) 
    index_l_r=index_l_r+delay2;

  else
    index_r_r=index_r_r+delay2;
  end
  
  index_end_l_l = index_l_l+l_out-1;  
  index_end_r_l = index_r_l+l_out-1;
  index_end_l_r = index_l_r+l_out-1;  
  index_end_r_r = index_r_r+l_out-1;
  
  y(1,index_l_l:index_end_l_l)= y(1,index_l_l:index_end_l_l) + conv(l_buff, reshape(hrir_l(az1,el,1:200),[1,200]));
  y(2,index_r_l:index_end_r_l)= y(2,index_r_l:index_end_r_l) + conv(l_buff, reshape(hrir_r(az1,el,1:200),[1,200]));
  
  y(1,index_l_r:index_end_l_r)= y(1,index_l_r:index_end_l_r) + conv(r_buff, reshape(hrir_l(az2,el,1:200),[1,200]));
  y(2,index_r_r:index_end_r_r)= y(2,index_r_r:index_end_r_r) + conv(r_buff, reshape(hrir_r(az2,el,1:200),[1,200]));
 
end

y=y.*0.8;
player1 = audioplayer(y,Fs);
player2 = audioplayer(x,Fs);

play(player1);



