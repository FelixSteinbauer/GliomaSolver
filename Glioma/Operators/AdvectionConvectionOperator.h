//
//  AdvectionConvectionOperator.h
//  GliomaXcode
//
//  Created by Lipkova on 21/05/15.
//  Copyright (c) 2015 Lipkova. All rights reserved.
//

#pragma once
#include "Glioma_Types.h"

using namespace MRAG;
using namespace  Science;

// Compute max velocity -> used to estimate advection restricted time step
template<int nDim = 3>
struct GetUMax
{
    map< int, Real>& local_max_velocities;
    
    GetUMax(map< int, Real>& local_max_velocities):local_max_velocities(local_max_velocities)
    { }
    
    GetUMax(const GetUMax& c): local_max_velocities(c.local_max_velocities)
    { }
    
    template<typename BlockType>
    inline void operator() (const BlockInfo& info, BlockType& b) const
    {
        Real maxVel[3] = {0,0,0};
        if(nDim == 2)
        {
            for(int iy=0; iy<BlockType::sizeY; iy++)
                for(int ix=0; ix<BlockType::sizeX; ix++)
                {
                    maxVel[0] = max((Real)fabs( b(ix,iy).ux), maxVel[0]);
                    maxVel[1] = max((Real)fabs( b(ix,iy).uy), maxVel[1]);
                }
        }
        else
        {
            for(int iz=0; iz<BlockType::sizeZ; iz++)
                for(int iy=0; iy<BlockType::sizeY; iy++)
                    for(int ix=0; ix<BlockType::sizeX; ix++)
                    {
                        maxVel[0] = max((Real)fabs( b(ix,iy,iz).ux), maxVel[0]);
                        maxVel[1] = max((Real)fabs( b(ix,iy,iz).uy), maxVel[1]);
                        maxVel[2] = max((Real)fabs( b(ix,iy,iz).uz), maxVel[2]);
                    }
        }
        
        map< int, Real>::iterator it = local_max_velocities.find(info.blockID);
        assert(it != local_max_velocities.end());
        
        it->second = max(maxVel[0], max(maxVel[1], maxVel[2]));
    }
};


template<int nDim = 3>
struct TissueTumorAdvectionWeno5
{
    int stencil_start[3];
    int stencil_end[3];
    
    TissueTumorAdvectionWeno5()
    {
        stencil_start[0]=stencil_start[1] = -3;
        stencil_end[0]  =stencil_end[1]   = +4;
        stencil_start[2] = nDim==3 ? -3: 0;
        stencil_end[2]   = nDim==3 ? +4:+1;
    }
    
    TissueTumorAdvectionWeno5(const TissueTumorAdvectionWeno5&)
    {
        stencil_start[0]=stencil_start[1] = -3;
        stencil_end[0]  =stencil_end[1]   = +4;
        stencil_start[2] = nDim==3 ? -3: 0;
        stencil_end[2]   = nDim==3 ? +4:+1;
        
    }
    
    template<typename LabType, typename BlockType>
    inline void operator()(LabType& lab, const BlockInfo& info, BlockType& o) const
    {
        Weno5<double> weno5;
        
        const double ifactor = 1./(info.h[0]);
        
        // fluxes
        double wm_flux_x[2];
        double wm_flux_y[2];
        double wm_flux_z[2];
        
        double gm_flux_x[2];
        double gm_flux_y[2];
        double gm_flux_z[2];
        
        double csf_flux_x[2];
        double csf_flux_y[2];
        double csf_flux_z[2];
        
        double tumor_flux_x[2];
        double tumor_flux_y[2];
        double tumor_flux_z[2];
        
        
        // n is domain charact. function -> used to apply bc
        Real n[6]; // neighbours [-3,-2,-1,1,2,3]
        Real tissue[6]; // used to makr brain tissue: wm+gm -> use for tumor BC at csf
        
        if(nDim==2)
        {
            for(int iy=0; iy<BlockType::sizeY; iy++)
                for(int ix=0; ix<BlockType::sizeX; ix++)
                {
                    if( lab(ix,iy).chi > 0)
                    {
                        /* In X */
                        n[0] = lab(ix-3,iy).chi;
                        n[1] = lab(ix-2,iy).chi;
                        n[2] = lab(ix-1,iy).chi;
                        n[3] = lab(ix+1,iy).chi;
                        n[4] = lab(ix+2,iy).chi;
                        n[5] = lab(ix+3,iy).chi;
                        
                         _applyNoFluxBC(n);
                        
                        // white matter
                        weno5(  ifactor * ( n[1] * lab(ix-2,iy).wm - n[0] * lab(ix-3,iy).wm ),
                              ifactor   * ( n[2] * lab(ix-1,iy).wm - n[1] * lab(ix-2,iy).wm ),
                              ifactor   * (        lab(ix  ,iy).wm - n[2] * lab(ix-1,iy).wm ),
                              ifactor   * ( n[3] * lab(ix+1,iy).wm -        lab(ix  ,iy).wm ),
                              ifactor   * ( n[4] * lab(ix+2,iy).wm - n[3] * lab(ix+1,iy).wm ),
                              ifactor   * ( n[5] * lab(ix+3,iy).wm - n[4] * lab(ix+2,iy).wm ),
                              wm_flux_x );
                        
                        // gray matter
                        weno5(  ifactor * ( n[1] * lab(ix-2,iy).gm - n[0] * lab(ix-3,iy).gm ),
                              ifactor   * ( n[2] * lab(ix-1,iy).gm - n[1] * lab(ix-2,iy).gm ),
                              ifactor   * (        lab(ix  ,iy).gm - n[2] * lab(ix-1,iy).gm ),
                              ifactor   * ( n[3] * lab(ix+1,iy).gm -        lab(ix  ,iy).gm ),
                              ifactor   * ( n[4] * lab(ix+2,iy).gm - n[3] * lab(ix+1,iy).gm ),
                              ifactor   * ( n[5] * lab(ix+3,iy).gm - n[4] * lab(ix+2,iy).gm ),
                              gm_flux_x );
                        
                        // csf
                        weno5(  ifactor * ( n[1] * lab(ix-2,iy).csf - n[0] * lab(ix-3,iy).csf ),
                              ifactor   * ( n[2] * lab(ix-1,iy).csf - n[1] * lab(ix-2,iy).csf ),
                              ifactor   * (        lab(ix  ,iy).csf - n[2] * lab(ix-1,iy).csf ),
                              ifactor   * ( n[3] * lab(ix+1,iy).csf -        lab(ix  ,iy).csf ),
                              ifactor   * ( n[4] * lab(ix+2,iy).csf - n[3] * lab(ix+1,iy).csf ),
                              ifactor   * ( n[5] * lab(ix+3,iy).csf - n[4] * lab(ix+2,iy).csf ),
                              csf_flux_x );
                        
                        // tumor
                        tissue[0] = lab(ix-3,iy).p_w + lab(ix-3,iy).p_g + lab(ix-3,iy).phi;
                        tissue[1] = lab(ix-2,iy).p_w + lab(ix-2,iy).p_g + lab(ix-2,iy).phi;
                        tissue[2] = lab(ix-1,iy).p_w + lab(ix-1,iy).p_g + lab(ix-1,iy).phi;
                        tissue[3] = lab(ix+1,iy).p_w + lab(ix+1,iy).p_g + lab(ix+1,iy).phi;
                        tissue[4] = lab(ix+2,iy).p_w + lab(ix+2,iy).p_g + lab(ix+2,iy).phi;
                        tissue[5] = lab(ix+3,iy).p_w + lab(ix+3,iy).p_g + lab(ix+3,iy).phi;
                        
                        n[0] = lab(ix-3,iy).p_w + lab(ix-3,iy).p_g;
                        n[1] = lab(ix-2,iy).p_w + lab(ix-2,iy).p_g;
                        n[2] = lab(ix-1,iy).p_w + lab(ix-1,iy).p_g;
                        n[3] = lab(ix+1,iy).p_w + lab(ix+1,iy).p_g;
                        n[4] = lab(ix+2,iy).p_w + lab(ix+2,iy).p_g;
                        n[5] = lab(ix+3,iy).p_w + lab(ix+3,iy).p_g;
                        
                        _applyNoFluxBCTumor(n,tissue);
                        
                        weno5(  ifactor * ( n[1] * lab(ix-2,iy).phi - n[0] * lab(ix-3,iy).phi ),
                              ifactor   * ( n[2] * lab(ix-1,iy).phi - n[1] * lab(ix-2,iy).phi ),
                              ifactor   * (        lab(ix  ,iy).phi - n[2] * lab(ix-1,iy).phi ),
                              ifactor   * ( n[3] * lab(ix+1,iy).phi -        lab(ix  ,iy).phi ),
                              ifactor   * ( n[4] * lab(ix+2,iy).phi - n[3] * lab(ix+1,iy).phi ),
                              ifactor   * ( n[5] * lab(ix+3,iy).phi - n[4] * lab(ix+2,iy).phi ),
                              tumor_flux_x );
                        
                        
                        /* In Y */
                        n[0] = lab(ix,iy-3).chi;
                        n[1] = lab(ix,iy-2).chi;
                        n[2] = lab(ix,iy-1).chi;
                        n[3] = lab(ix,iy+1).chi;
                        n[4] = lab(ix,iy+2).chi;
                        n[5] = lab(ix,iy+3).chi;
                        
                        _applyNoFluxBC(n);
                        
                        // white matter
                        weno5( ifactor * ( n[1] * lab(ix,iy-2).wm - n[0] * lab(ix,iy-3).wm ),
                              ifactor  * ( n[2] * lab(ix,iy-1).wm - n[1] * lab(ix,iy-2).wm ),
                              ifactor  * (        lab(ix,iy  ).wm - n[2] * lab(ix,iy-1).wm ),
                              ifactor  * ( n[3] * lab(ix,iy+1).wm -        lab(ix,iy  ).wm ),
                              ifactor  * ( n[4] * lab(ix,iy+2).wm - n[3] * lab(ix,iy+1).wm ),
                              ifactor  * ( n[5] * lab(ix,iy+3).wm - n[4] * lab(ix,iy+2).wm ),
                              wm_flux_y );
                        
                        // gray matter
                        weno5( ifactor * ( n[1] * lab(ix,iy-2).gm - n[0] * lab(ix,iy-3).gm ),
                              ifactor  * ( n[2] * lab(ix,iy-1).gm - n[1] * lab(ix,iy-2).gm ),
                              ifactor  * (        lab(ix,iy  ).gm - n[2] * lab(ix,iy-1).gm ),
                              ifactor  * ( n[3] * lab(ix,iy+1).gm -        lab(ix,iy  ).gm ),
                              ifactor  * ( n[4] * lab(ix,iy+2).gm - n[3] * lab(ix,iy+1).gm ),
                              ifactor  * ( n[5] * lab(ix,iy+3).gm - n[4] * lab(ix,iy+2).gm ),
                              gm_flux_y );
                        
                        // csf
                        weno5( ifactor * ( n[1] * lab(ix,iy-2).csf - n[0] * lab(ix,iy-3).csf ),
                              ifactor  * ( n[2] * lab(ix,iy-1).csf - n[1] * lab(ix,iy-2).csf ),
                              ifactor  * (        lab(ix,iy  ).csf - n[2] * lab(ix,iy-1).csf ),
                              ifactor  * ( n[3] * lab(ix,iy+1).csf -        lab(ix,iy  ).csf ),
                              ifactor  * ( n[4] * lab(ix,iy+2).csf - n[3] * lab(ix,iy+1).csf ),
                              ifactor  * ( n[5] * lab(ix,iy+3).csf - n[4] * lab(ix,iy+2).csf ),
                              csf_flux_y );
                        
                        // tumor
                        tissue[0] = lab(ix,iy-3).p_w + lab(ix,iy-3).p_g + lab(ix,iy-3).phi;
                        tissue[1] = lab(ix,iy-2).p_w + lab(ix,iy-2).p_g + lab(ix,iy-2).phi;
                        tissue[2] = lab(ix,iy-1).p_w + lab(ix,iy-1).p_g + lab(ix,iy-1).phi;
                        tissue[3] = lab(ix,iy+1).p_w + lab(ix,iy+1).p_g + lab(ix,iy+1).phi;
                        tissue[4] = lab(ix,iy+2).p_w + lab(ix,iy+2).p_g + lab(ix,iy+2).phi;
                        tissue[5] = lab(ix,iy+3).p_w + lab(ix,iy+3).p_g + lab(ix,iy+3).phi;
                        
                        n[0] = lab(ix,iy-3).p_w + lab(ix,iy-3).p_g;
                        n[1] = lab(ix,iy-2).p_w + lab(ix,iy-2).p_g;
                        n[2] = lab(ix,iy-1).p_w + lab(ix,iy-1).p_g;
                        n[3] = lab(ix,iy+1).p_w + lab(ix,iy+1).p_g;
                        n[4] = lab(ix,iy+2).p_w + lab(ix,iy+2).p_g;
                        n[5] = lab(ix,iy+3).p_w + lab(ix,iy+3).p_g;
                        
                        _applyNoFluxBCTumor(n,tissue);
                        
                        weno5( ifactor * ( n[1] * lab(ix,iy-2).phi - n[0] * lab(ix,iy-3).phi ),
                              ifactor  * ( n[2] * lab(ix,iy-1).phi - n[1] * lab(ix,iy-2).phi ),
                              ifactor  * (        lab(ix,iy  ).phi - n[2] * lab(ix,iy-1).phi ),
                              ifactor  * ( n[3] * lab(ix,iy+1).phi -        lab(ix,iy  ).phi ),
                              ifactor  * ( n[4] * lab(ix,iy+2).phi - n[3] * lab(ix,iy+1).phi ),
                              ifactor  * ( n[5] * lab(ix,iy+3).phi - n[4] * lab(ix,iy+2).phi ),
                              tumor_flux_y );
                        
                        // velocities
                        Real velocity_plus[2] = {
                            max(o(ix,iy).ux, (Real) 0.),
                            max(o(ix,iy).uy, (Real) 0.)
                        };
                        
                        Real velocity_minus[2] = {
                            min(lab(ix,iy).ux, (Real) 0.),
                            min(lab(ix,iy).uy, (Real) 0.)
                        };
                        
                        o(ix,iy).dwmdt  = (-1.) * ( velocity_plus[0] * wm_flux_x[0] + velocity_minus[0] * wm_flux_x[1] +
                                                   velocity_plus[1] * wm_flux_y[0] + velocity_minus[1] * wm_flux_y[1]  );
                        
                        o(ix,iy).dgmdt  = (-1.) * ( velocity_plus[0] * gm_flux_x[0] + velocity_minus[0] * gm_flux_x[1] +
                                                   velocity_plus[1] * gm_flux_y[0] + velocity_minus[1] * gm_flux_y[1]  );
                        
                        o(ix,iy).dcsfdt = (-1.) * ( velocity_plus[0] * csf_flux_x[0] + velocity_minus[0] * csf_flux_x[1] +
                                                   velocity_plus[1] * csf_flux_y[0] + velocity_minus[1] * csf_flux_y[1]  );
                        
                        // since dphidt contains already the reaction diffusion step, add advection term to it
                        o(ix,iy).dphidt += (-1.) * ( velocity_plus[0] * tumor_flux_x[0] + velocity_minus[0] * tumor_flux_x[1] +
                                                   velocity_plus[1] * tumor_flux_y[0] + velocity_minus[1] * tumor_flux_y[1]  );
                        
                    }
                    else
                    {
                        o(ix,iy).dwmdt  = 0.;
                        o(ix,iy).dgmdt  = 0.;
                        o(ix,iy).dcsfdt = 0.;                        
                    }
                    
                }
        }
        else
        {
            for(int iz=0; iz<BlockType::sizeZ; iz++)
                for(int iy=0; iy<BlockType::sizeY; iy++)
                    for(int ix=0; ix<BlockType::sizeX; ix++)
                    {
                        if( lab(ix,iy,iz).chi > 0)
                        {
                            /* In X */
                            n[0] = lab(ix-3,iy,iz).chi;
                            n[1] = lab(ix-2,iy,iz).chi;
                            n[2] = lab(ix-1,iy,iz).chi;
                            n[3] = lab(ix+1,iy,iz).chi;
                            n[4] = lab(ix+2,iy,iz).chi;
                            n[5] = lab(ix+3,iy,iz).chi;
                            
                            _applyNoFluxBC(n);
                            
                            // white matter
                            weno5(  ifactor * ( n[1] * lab(ix-2,iy,iz).wm - n[0] * lab(ix-3,iy,iz).wm ),
                                  ifactor   * ( n[2] * lab(ix-1,iy,iz).wm - n[1] * lab(ix-2,iy,iz).wm ),
                                  ifactor   * (        lab(ix  ,iy,iz).wm - n[2] * lab(ix-1,iy,iz).wm ),
                                  ifactor   * ( n[3] * lab(ix+1,iy,iz).wm -        lab(ix  ,iy,iz).wm ),
                                  ifactor   * ( n[4] * lab(ix+2,iy,iz).wm - n[3] * lab(ix+1,iy,iz).wm ),
                                  ifactor   * ( n[5] * lab(ix+3,iy,iz).wm - n[4] * lab(ix+2,iy,iz).wm ),
                                  wm_flux_x );
                            
                            // gray matter
                            weno5(  ifactor * ( n[1] * lab(ix-2,iy,iz).gm - n[0] * lab(ix-3,iy,iz).gm ),
                                  ifactor   * ( n[2] * lab(ix-1,iy,iz).gm - n[1] * lab(ix-2,iy,iz).gm ),
                                  ifactor   * (        lab(ix  ,iy,iz).gm - n[2] * lab(ix-1,iy,iz).gm ),
                                  ifactor   * ( n[3] * lab(ix+1,iy,iz).gm -        lab(ix  ,iy,iz).gm ),
                                  ifactor   * ( n[4] * lab(ix+2,iy,iz).gm - n[3] * lab(ix+1,iy,iz).gm ),
                                  ifactor   * ( n[5] * lab(ix+3,iy,iz).gm - n[4] * lab(ix+2,iy,iz).gm ),
                                  gm_flux_x );
                            
                            // csf
                            weno5(  ifactor * ( n[1] * lab(ix-2,iy,iz).csf - n[0] * lab(ix-3,iy,iz).csf ),
                                  ifactor   * ( n[2] * lab(ix-1,iy,iz).csf - n[1] * lab(ix-2,iy,iz).csf ),
                                  ifactor   * (        lab(ix  ,iy,iz).csf - n[2] * lab(ix-1,iy,iz).csf ),
                                  ifactor   * ( n[3] * lab(ix+1,iy,iz).csf -        lab(ix  ,iy,iz).csf ),
                                  ifactor   * ( n[4] * lab(ix+2,iy,iz).csf - n[3] * lab(ix+1,iy,iz).csf ),
                                  ifactor   * ( n[5] * lab(ix+3,iy,iz).csf - n[4] * lab(ix+2,iy,iz).csf ),
                                  csf_flux_x );
                           
                            /* In X for Tumour*/
                            // tissue - defines domain for tumor advection
                            // n[] is where the BC is applied, the idea is: the more CSF at voxel -> the less tumor is advected there
                            tissue[0] = lab(ix-3,iy,iz).p_w + lab(ix-3,iy,iz).p_g + lab(ix-3,iy,iz).phi;
                            tissue[1] = lab(ix-2,iy,iz).p_w + lab(ix-2,iy,iz).p_g + lab(ix-2,iy,iz).phi;
                            tissue[2] = lab(ix-1,iy,iz).p_w + lab(ix-1,iy,iz).p_g + lab(ix-1,iy,iz).phi;
                            tissue[3] = lab(ix+1,iy,iz).p_w + lab(ix+1,iy,iz).p_g + lab(ix+1,iy,iz).phi;
                            tissue[4] = lab(ix+2,iy,iz).p_w + lab(ix+2,iy,iz).p_g + lab(ix+2,iy,iz).phi;
                            tissue[5] = lab(ix+3,iy,iz).p_w + lab(ix+3,iy,iz).p_g + lab(ix+3,iy,iz).phi;
                            
                            n[0] = lab(ix-3,iy,iz).p_w + lab(ix-3,iy,iz).p_g;
                            n[1] = lab(ix-2,iy,iz).p_w + lab(ix-2,iy,iz).p_g;
                            n[2] = lab(ix-1,iy,iz).p_w + lab(ix-1,iy,iz).p_g;
                            n[3] = lab(ix+1,iy,iz).p_w + lab(ix+1,iy,iz).p_g;
                            n[4] = lab(ix+2,iy,iz).p_w + lab(ix+2,iy,iz).p_g;
                            n[5] = lab(ix+3,iy,iz).p_w + lab(ix+3,iy,iz).p_g;
                            
                            _applyNoFluxBCTumor(n,tissue);
                            
                            weno5(  ifactor * ( n[1] * lab(ix-2,iy,iz).phi - n[0] * lab(ix-3,iy,iz).phi ),
                                  ifactor   * ( n[2] * lab(ix-1,iy,iz).phi - n[1] * lab(ix-2,iy,iz).phi ),
                                  ifactor   * (        lab(ix  ,iy,iz).phi - n[2] * lab(ix-1,iy,iz).phi ),
                                  ifactor   * ( n[3] * lab(ix+1,iy,iz).phi -        lab(ix  ,iy,iz).phi ),
                                  ifactor   * ( n[4] * lab(ix+2,iy,iz).phi - n[3] * lab(ix+1,iy,iz).phi ),
                                  ifactor   * ( n[5] * lab(ix+3,iy,iz).phi - n[4] * lab(ix+2,iy,iz).phi ),
                                  tumor_flux_x );
                            
                            
                            /* In Y */
                            n[0] = lab(ix,iy-3,iz).chi;
                            n[1] = lab(ix,iy-2,iz).chi;
                            n[2] = lab(ix,iy-1,iz).chi;
                            n[3] = lab(ix,iy+1,iz).chi;
                            n[4] = lab(ix,iy+2,iz).chi;
                            n[5] = lab(ix,iy+3,iz).chi;
                            
                             _applyNoFluxBC(n);
                            
                            // white matter
                            weno5( ifactor * ( n[1] * lab(ix,iy-2,iz).wm - n[0] * lab(ix,iy-3,iz).wm ),
                                  ifactor  * ( n[2] * lab(ix,iy-1,iz).wm - n[1] * lab(ix,iy-2,iz).wm ),
                                  ifactor  * (        lab(ix,iy  ,iz).wm - n[2] * lab(ix,iy-1,iz).wm ),
                                  ifactor  * ( n[3] * lab(ix,iy+1,iz).wm -        lab(ix,iy  ,iz).wm ),
                                  ifactor  * ( n[4] * lab(ix,iy+2,iz).wm - n[3] * lab(ix,iy+1,iz).wm ),
                                  ifactor  * ( n[5] * lab(ix,iy+3,iz).wm - n[4] * lab(ix,iy+2,iz).wm ),
                                  wm_flux_y );
                            
                            // gray matter
                            weno5( ifactor * ( n[1] * lab(ix,iy-2,iz).gm - n[0] * lab(ix,iy-3,iz).gm ),
                                  ifactor  * ( n[2] * lab(ix,iy-1,iz).gm - n[1] * lab(ix,iy-2,iz).gm ),
                                  ifactor  * (        lab(ix,iy  ,iz).gm - n[2] * lab(ix,iy-1,iz).gm ),
                                  ifactor  * ( n[3] * lab(ix,iy+1,iz).gm -        lab(ix,iy  ,iz).gm ),
                                  ifactor  * ( n[4] * lab(ix,iy+2,iz).gm - n[3] * lab(ix,iy+1,iz).gm ),
                                  ifactor  * ( n[5] * lab(ix,iy+3,iz).gm - n[4] * lab(ix,iy+2,iz).gm ),
                                  gm_flux_y );
                            
                            // csf
                            weno5( ifactor * ( n[1] * lab(ix,iy-2,iz).csf - n[0] * lab(ix,iy-3,iz).csf ),
                                  ifactor  * ( n[2] * lab(ix,iy-1,iz).csf - n[1] * lab(ix,iy-2,iz).csf ),
                                  ifactor  * (        lab(ix,iy  ,iz).csf - n[2] * lab(ix,iy-1,iz).csf ),
                                  ifactor  * ( n[3] * lab(ix,iy+1,iz).csf -        lab(ix,iy  ,iz).csf ),
                                  ifactor  * ( n[4] * lab(ix,iy+2,iz).csf - n[3] * lab(ix,iy+1,iz).csf ),
                                  ifactor  * ( n[5] * lab(ix,iy+3,iz).csf - n[4] * lab(ix,iy+2,iz).csf ),
                                  csf_flux_y );
                           
                            /* In Y for Tumour*/
                            tissue[0] = lab(ix,iy-3,iz).p_w + lab(ix,iy-3,iz).p_g + lab(ix,iy-3,iz).phi;
                            tissue[1] = lab(ix,iy-2,iz).p_w + lab(ix,iy-2,iz).p_g + lab(ix,iy-2,iz).phi;
                            tissue[2] = lab(ix,iy-1,iz).p_w + lab(ix,iy-1,iz).p_g + lab(ix,iy-1,iz).phi;
                            tissue[3] = lab(ix,iy+1,iz).p_w + lab(ix,iy+1,iz).p_g + lab(ix,iy+1,iz).phi;
                            tissue[4] = lab(ix,iy+2,iz).p_w + lab(ix,iy+2,iz).p_g + lab(ix,iy+2,iz).phi;
                            tissue[5] = lab(ix,iy+3,iz).p_w + lab(ix,iy+3,iz).p_g + lab(ix,iy+3,iz).phi;
                            
                            n[0] = lab(ix,iy-3,iz).p_w + lab(ix,iy-3,iz).p_g;
                            n[1] = lab(ix,iy-2,iz).p_w + lab(ix,iy-2,iz).p_g;
                            n[2] = lab(ix,iy-1,iz).p_w + lab(ix,iy-1,iz).p_g;
                            n[3] = lab(ix,iy+1,iz).p_w + lab(ix,iy+1,iz).p_g;
                            n[4] = lab(ix,iy+2,iz).p_w + lab(ix,iy+2,iz).p_g;
                            n[5] = lab(ix,iy+3,iz).p_w + lab(ix,iy+3,iz).p_g;
                            
                            _applyNoFluxBCTumor(n,tissue);
                            
                            // tumor
                            weno5( ifactor * ( n[1] * lab(ix,iy-2,iz).phi - n[0] * lab(ix,iy-3,iz).phi ),
                                  ifactor  * ( n[2] * lab(ix,iy-1,iz).phi - n[1] * lab(ix,iy-2,iz).phi ),
                                  ifactor  * (        lab(ix,iy  ,iz).phi - n[2] * lab(ix,iy-1,iz).phi ),
                                  ifactor  * ( n[3] * lab(ix,iy+1,iz).phi -        lab(ix,iy  ,iz).phi ),
                                  ifactor  * ( n[4] * lab(ix,iy+2,iz).phi - n[3] * lab(ix,iy+1,iz).phi ),
                                  ifactor  * ( n[5] * lab(ix,iy+3,iz).phi - n[4] * lab(ix,iy+2,iz).phi ),
                                  tumor_flux_y );
                            
                            
                            /* In Z */
                            n[0] = lab(ix,iy,iz-3).chi;
                            n[1] = lab(ix,iy,iz-2).chi;
                            n[2] = lab(ix,iy,iz-1).chi;
                            n[3] = lab(ix,iy,iz+1).chi;
                            n[4] = lab(ix,iy,iz+2).chi;
                            n[5] = lab(ix,iy,iz+3).chi;
                            
                            _applyNoFluxBC(n);
                            
                            // white matter
                            weno5( ifactor * ( n[1] * lab(ix,iy,iz-2).wm - n[0] * lab(ix,iy,iz-3).wm ),
                                  ifactor  * ( n[2] * lab(ix,iy,iz-1).wm - n[1] * lab(ix,iy,iz-2).wm ),
                                  ifactor  * (        lab(ix,iy,iz  ).wm - n[2] * lab(ix,iy,iz-1).wm ),
                                  ifactor  * ( n[3] * lab(ix,iy,iz+1).wm -        lab(ix,iy,iz  ).wm ),
                                  ifactor  * ( n[4] * lab(ix,iy,iz+2).wm - n[3] * lab(ix,iy,iz+1).wm ),
                                  ifactor  * ( n[5] * lab(ix,iy,iz+3).wm - n[4] * lab(ix,iy,iz+2).wm ),
                                  wm_flux_z );
                            
                            // gray matter
                            weno5( ifactor * ( n[1] * lab(ix,iy,iz-2).gm - n[0] * lab(ix,iy,iz-3).gm ),
                                  ifactor  * ( n[2] * lab(ix,iy,iz-1).gm - n[1] * lab(ix,iy,iz-2).gm ),
                                  ifactor  * (        lab(ix,iy  ,iz).gm - n[2] * lab(ix,iy,iz-1).gm ),
                                  ifactor  * ( n[3] * lab(ix,iy,iz+1).gm -        lab(ix,iy,iz  ).gm ),
                                  ifactor  * ( n[4] * lab(ix,iy,iz+2).gm - n[3] * lab(ix,iy,iz+1).gm ),
                                  ifactor  * ( n[5] * lab(ix,iy,iz+3).gm - n[4] * lab(ix,iy,iz+2).gm ),
                                  gm_flux_z );
                            
                            // csf
                            weno5( ifactor * ( n[1] * lab(ix,iy,iz-2).csf - n[0] * lab(ix,iy,iz-3).csf ),
                                  ifactor  * ( n[2] * lab(ix,iy,iz-1).csf - n[1] * lab(ix,iy,iz-2).csf ),
                                  ifactor  * (        lab(ix,iy,iz  ).csf - n[2] * lab(ix,iy,iz-1).csf ),
                                  ifactor  * ( n[3] * lab(ix,iy,iz+1).csf -        lab(ix,iy,iz  ).csf ),
                                  ifactor  * ( n[4] * lab(ix,iy,iz+2).csf - n[3] * lab(ix,iy,iz+1).csf ),
                                  ifactor  * ( n[5] * lab(ix,iy,iz+3).csf - n[4] * lab(ix,iy,iz+2).csf ),
                                  csf_flux_z );
                            

                            /* In Z for tumour */
                            tissue[0] = lab(ix,iy,iz-3).p_w + lab(ix,iy,iz-3).p_g + lab(ix,iy,iz-3).phi;
                            tissue[1] = lab(ix,iy,iz-2).p_w + lab(ix,iy,iz-2).p_g + lab(ix,iy,iz-2).phi;
                            tissue[2] = lab(ix,iy,iz-1).p_w + lab(ix,iy,iz-1).p_g + lab(ix,iy,iz-1).phi;
                            tissue[3] = lab(ix,iy,iz+1).p_w + lab(ix,iy,iz+1).p_g + lab(ix,iy,iz+1).phi;
                            tissue[4] = lab(ix,iy,iz+2).p_w + lab(ix,iy,iz+2).p_g + lab(ix,iy,iz+2).phi;
                            tissue[5] = lab(ix,iy,iz+3).p_w + lab(ix,iy,iz+3).p_g + lab(ix,iy,iz+3).phi;
                            
                            n[0] = lab(ix,iy,iz-3).p_w + lab(ix,iy,iz-3).p_g;
                            n[1] = lab(ix,iy,iz-2).p_w + lab(ix,iy,iz-2).p_g;
                            n[2] = lab(ix,iy,iz-1).p_w + lab(ix,iy,iz-1).p_g;
                            n[3] = lab(ix,iy,iz+1).p_w + lab(ix,iy,iz+1).p_g;
                            n[4] = lab(ix,iy,iz+2).p_w + lab(ix,iy,iz+2).p_g;
                            n[5] = lab(ix,iy,iz+3).p_w + lab(ix,iy,iz+3).p_g;
                            
                            _applyNoFluxBCTumor(n,tissue);
                            
                            // tumor
                            weno5( ifactor * ( n[1] * lab(ix,iy,iz-2).phi - n[0] * lab(ix,iy,iz-3).phi ),
                                  ifactor  * ( n[2] * lab(ix,iy,iz-1).phi - n[1] * lab(ix,iy,iz-2).phi ),
                                  ifactor  * (        lab(ix,iy,iz  ).phi - n[2] * lab(ix,iy,iz-1).phi ),
                                  ifactor  * ( n[3] * lab(ix,iy,iz+1).phi -        lab(ix,iy,iz  ).phi ),
                                  ifactor  * ( n[4] * lab(ix,iy,iz+2).phi - n[3] * lab(ix,iy,iz+1).phi ),
                                  ifactor  * ( n[5] * lab(ix,iy,iz+3).phi - n[4] * lab(ix,iy,iz+2).phi ),
                                  tumor_flux_z );
                            
                            /* Velocities */
                            Real velocity_plus[3] = {
                                max(o(ix,iy,iz).ux, (Real) 0.),
                                max(o(ix,iy,iz).uy, (Real) 0.),
                                max(o(ix,iy,iz).uz, (Real) 0.)
                            };
                            
                            Real velocity_minus[3] = {
                                min(lab(ix,iy,iz).ux, (Real) 0.),
                                min(lab(ix,iy,iz).uy, (Real) 0.),
                                min(lab(ix,iy,iz).uz, (Real) 0.)
                                
                            };
                            
                            o(ix,iy,iz).dwmdt   = (-1.) * ( velocity_plus[0] * wm_flux_x[0] + velocity_minus[0] * wm_flux_x[1] +
                                                          velocity_plus[1] * wm_flux_y[0] + velocity_minus[1] * wm_flux_y[1] +
                                                          velocity_plus[2] * wm_flux_z[0] + velocity_minus[2] * wm_flux_z[1] );
                            
                            o(ix,iy,iz).dgmdt   = (-1.) * ( velocity_plus[0] * gm_flux_x[0] + velocity_minus[0] * gm_flux_x[1] +
                                                          velocity_plus[1] * gm_flux_y[0] + velocity_minus[1] * gm_flux_y[1] +
                                                          velocity_plus[2] * gm_flux_z[0] + velocity_minus[2] * gm_flux_z[1] );
                            
                            o(ix,iy,iz).dcsfdt  = (-1.) * ( velocity_plus[0] * csf_flux_x[0] + velocity_minus[0] * csf_flux_x[1] +
                                                          velocity_plus[1] * csf_flux_y[0] + velocity_minus[1] * csf_flux_y[1] +
                                                          velocity_plus[2] * csf_flux_z[0] + velocity_minus[2] * csf_flux_z[1]);
                            
                            // dphidt already contains reaction-diffusion term, updated do not overwrite !
                            o(ix,iy,iz).dphidt += (-1.) * ( velocity_plus[0] * tumor_flux_x[0] + velocity_minus[0] * tumor_flux_x[1] +
                                                          velocity_plus[1] * tumor_flux_y[0] + velocity_minus[1] * tumor_flux_y[1] +
                                                          velocity_plus[2] * tumor_flux_z[0] + velocity_minus[2] * tumor_flux_z[1]);
                            
                        }
                        else
                        {
                            o(ix,iy,iz).dwmdt  = 0.;
                            o(ix,iy,iz).dgmdt  = 0.;
                            o(ix,iy,iz).dcsfdt = 0.;
                        }
                        
                    }
        }
    }
    
    
    inline void _applyNoFluxBC(Real (&n)[6] ) const
    {
        // n domain char. fun. n = 0 -> outside domain (apply bc), n>=1 -> inside domain
        Real eps = 0.1;
        if( n[0] < eps ){n[5] *= 2.; };
        if( n[1] < eps ){n[4] *= 2.; };
        if( n[2] < eps ){n[3] *= 2.; };
        if( n[3] < eps ){n[2] *= 2.; };
        if( n[4] < eps ){n[1] *= 2.; };
        if( n[5] < eps ){n[0] *= 2.; };
    }
    
    inline void _applyNoFluxBCTumor(Real (&n)[6], Real tissue[6] ) const
    {
        // tissue - defines domain, n - is contains the values to which to apply the BC
        // e.g. tissue > 0 -> wm+gm+tum  (can be >1, just used to mark the domain), tissue = 0 -> BC, n = p_w + p_g
        Real eps = 0.1;
        if( tissue[0] < eps ){n[5] *= 2.; };
        if( tissue[1] < eps ){n[4] *= 2.; };
        if( tissue[2] < eps ){n[3] *= 2.; };
        if( tissue[3] < eps ){n[2] *= 2.; };
        if( tissue[4] < eps ){n[1] *= 2.; };
        if( tissue[5] < eps ){n[0] *= 2.; };
    }
    
};


template<int nDim = 3>
struct TissueConvection
{
    int stencil_start[3];
    int stencil_end[3];
    
    TissueConvection()
    {
        stencil_start[0] = stencil_start[1] = -1;
        stencil_end[0]   = stencil_end[1]   = +2;
        stencil_start[2] = nDim==3 ? -1: 0;
        stencil_end[2]   = nDim==3 ? +2:+1;
    }
    
    TissueConvection(const TissueConvection&)
    {
        stencil_start[0] = stencil_start[1] = -1;
        stencil_end[0]   = stencil_end[1]   = +2;
        stencil_start[2] = nDim==3 ? -1: 0;
        stencil_end[2]   = nDim==3 ? +2:+1;
        
    }
    
    template<typename LabType, typename BlockType>
    inline void operator()(LabType& lab, const BlockInfo& info, BlockType& o) const
    {
        const double ih = 1./(info.h[0]);
        
        if(nDim == 2)
        {
            for(int iy=0; iy<BlockType::sizeY; iy++)
                for(int ix=0; ix<BlockType::sizeX; ix++)
                {
                    if( lab(ix,iy).chi > 0)
                    {
                        Real uym = lab(ix  ,iy-1).uy;
                        Real uyp = lab(ix  ,iy+1).uy;
                        Real uy  = lab(ix,iy).uy;
                        
                        Real uxm = lab(ix-1,iy  ).ux;
                        Real uxp = lab(ix+1,iy  ).ux;
                        Real ux = lab(ix,iy).ux;
                        
                        _mean(uy,uym,uyp);
                        _mean(ux,uxm,uxp);
                        
                        Real div_v = ih * (uxp - uxm + uyp - uym);
                        
                        o(ix,iy).dwmdt  -= lab(ix,iy).wm  * div_v;
                        o(ix,iy).dgmdt  -= lab(ix,iy).gm  * div_v;
                        o(ix,iy).dcsfdt -= lab(ix,iy).csf * div_v;
                        //o(ix,iy).dphidt -= lab(ix,iy).phi * div_v; // uncoment if tumor compression is considered
                    }
                    
                }
        }
        else
        {
            for(int iz=0; iz<BlockType::sizeZ; iz++)
                for(int iy=0; iy<BlockType::sizeY; iy++)
                    for(int ix=0; ix<BlockType::sizeX; ix++)
                    {
                        if( lab(ix,iy,iz).chi > 0)
                        {
                            Real uzm = lab(ix  ,iy  ,iz-1).uz * lab(ix  ,iy  ,iz-1).chi;
                            Real uzp = lab(ix  ,iy  ,iz+1).uz * lab(ix  ,iy  ,iz+1).chi;
                            Real uz  = lab(ix,  iy,  iz  ).uz * lab(ix,  iy,  iz  ).chi;
                            
                            Real uym = lab(ix  ,iy-1,iz  ).uy * lab(ix  ,iy-1,iz  ).chi;
                            Real uyp = lab(ix  ,iy+1,iz  ).uy * lab(ix  ,iy+1,iz  ).chi;
                            Real uy  = lab(ix,  iy,  iz  ).uy * lab(ix,  iy,   iz ).chi;
                            
                            Real uxm = lab(ix-1,iy  ,iz  ).ux * lab(ix-1,iy  ,iz  ).chi;
                            Real uxp = lab(ix+1,iy  ,iz  ).ux * lab(ix+1,iy  ,iz  ).chi;
                            Real ux  = lab(ix,  iy,  iz  ).ux * lab(ix,  iy,  iz  ).chi;
                            
                            Real div_v = 0.5 * ih * (uxp - uxm + uyp - uym + uzp - uzm);
                            
                            o(ix,iy,iz).dwmdt  -= lab(ix,iy,iz).wm  * div_v;
                            o(ix,iy,iz).dgmdt  -= lab(ix,iy,iz).gm  * div_v;
                            o(ix,iy,iz).dcsfdt -= lab(ix,iy,iz).csf * div_v;
                            //o(ix,iy,iz).dphidt -= lab(ix,iy).phi * div_v;  // uncoment if tumor compression is considered
                        }
                        
                    }
        }
    }
    
    
    inline void _mean(Real u, Real& up, Real& um) const
    {
        up = 0.5 * (u + up);
        um = 0.5 * (u + um);
    }
    
};


