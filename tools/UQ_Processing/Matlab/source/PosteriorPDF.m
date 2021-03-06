%===========================================================
%  
%                     plot posterior PDF 
% ---------------------------------------------------------
%  INPUT:  Path to file with posterior samples from TMCMC
%  OUTPUT: Figure with posterior pdf
%
%  Created by Lipkova on 22/11/18.
%  Copyright (c) 2018 Lipkova
%===================================  


function PosteriorPDF(InputPath)

addpath('../lib/jbfill')
addpath('../lib/tightfig')
addpath('../lib/freezeColors')
addpath('../lib/tmcmcTools')
addpath('../lib')
path2output=InputPath(1:end-13);

names =       ['D   ' ;'rho '  ;'Tend' ;'ix  ';'iy  ';'iz  ';'PETn';'b   ';'T1uc' ;'T2uc';'Tn  '];
bSynthetic = 0;
groundTruth = [ 1.3e-03 , 2.5e-02, 302 , 0.28, 0.67,   0.42,  0.0624,  0.801831, 0.7,   0.25,  5.0e-02  ];


mydata = importdata(InputPath);
[Nx,Ny] = size(mydata);


% Generic prio range
B0  = [ 0.0002   0.0130 ];
B1  = [ 0.0027   0.1900 ];
B2  = [ 30.0     1500.0 ];
B3  = [ 0.0      1.0    ];
B4  = [ 0.0      1.0    ];
B5  = [ 0.0      1.0    ];
B6  = [ 0.018    0.25   ];
B7  = [ 0.6      1.02   ];
B8  = [ 0.6      0.8    ];
B9  = [ 0.05     0.6    ];
B10 = [ 0.05     0.08   ];

bounds = [B0;B1;B2;B3;B4;B5;B6;B7;B8;B9;B10];

KDEbins = 20*ones(1,Ny-1);
Nbins   = 200;
dump = 1;
param= 1:Ny-1 ;
plotPosteriorTMCMC(mydata,param,bounds,names,groundTruth,dump,KDEbins,Nbins,bSynthetic,path2output)




