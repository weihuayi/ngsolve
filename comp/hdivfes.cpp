/*********************************************************************/
/* File:   hdivfes.cpp                                               */
/* Author: Joachim Schoeberl                                         */
/* Date:   27. Jan. 2003                                             */
/*********************************************************************/

/* 
   Finite Element Space for H(div)
*/

#include <comp.hpp>
#include <multigrid.hpp>

#include <../fem/hdivhofe.hpp>  
#include <../fem/hdivlofe.hpp>

namespace ngcomp
{
  using namespace ngcomp;

  /*
  RaviartThomasFESpace :: 
  RaviartThomasFESpace (shared_ptr<MeshAccess> ama,
			int adim, bool acomplex)
    
    : FESpace (ama, 1, adim, acomplex)
  {
    name="RaviartThomasFESpace(hdiv)";
    
    trig    = new FE_RTTrig0;
    segm    = new HDivNormalSegm0;

    if (ma.GetDimension() == 2)
      {
	Array<CoefficientFunction*> coeffs(1);
	coeffs[0] = new ConstantCoefficientFunction(1);
	evaluator = GetIntegrators().CreateBFI("masshdiv", 2, coeffs);
      }
  }
  */

  RaviartThomasFESpace :: RaviartThomasFESpace (shared_ptr<MeshAccess> ama, const Flags& flags, bool parseflags)
  : FESpace (ama, flags)
  {
    name="RaviartThomasFESpace(hdiv)";
    // defined flags
    DefineDefineFlag("hdiv");
    
    // parse flags
    if(parseflags) CheckFlags(flags);
    
    order = 1; // he: see above constructor!
        
    // trig    = new FE_RTTrig0;
    // segm    = new HDivNormalSegm0;

    // SetDummyFE<HDivDummyFE> ();
    
    if (ma->GetDimension() == 2)
    {
      Array<shared_ptr<CoefficientFunction>> coeffs(1);
      coeffs[0] = shared_ptr<CoefficientFunction> (new ConstantCoefficientFunction(1));
      integrator[VOL] = GetIntegrators().CreateBFI("masshdiv", 2, coeffs);
    }
    if (ma->GetDimension() == 3)
    {
      Array<shared_ptr<CoefficientFunction>> coeffs(1);
      coeffs[0] = shared_ptr<CoefficientFunction> (new ConstantCoefficientFunction(1));
      integrator[VOL] = GetIntegrators().CreateBFI("masshdiv", 3, coeffs);
    }
    if (ma->GetDimension() == 2)
      {
        evaluator[VOL] = make_shared<T_DifferentialOperator<DiffOpIdHDiv<2>>>();
        evaluator[BND] = make_shared<T_DifferentialOperator<DiffOpIdVecHDivBoundary<2>>>();
        flux_evaluator[VOL] = make_shared<T_DifferentialOperator<DiffOpDivHDiv<2>>>();
      }
    else
      {
        evaluator[VOL] = make_shared<T_DifferentialOperator<DiffOpIdHDiv<3>>>();
        evaluator[BND] = make_shared<T_DifferentialOperator<DiffOpIdVecHDivBoundary<3>>>();
        flux_evaluator[VOL] = make_shared<T_DifferentialOperator<DiffOpDivHDiv<3>>>();
      }
  }
      
    RaviartThomasFESpace :: ~RaviartThomasFESpace ()
  {
    ;
  }


  shared_ptr<FESpace> RaviartThomasFESpace :: 
  Create (shared_ptr<MeshAccess> ma, const Flags & flags)
  {
    int order = int(flags.GetNumFlag ("order", 0));

    if (order <= 0)
      return make_shared<RaviartThomasFESpace> (ma, flags, true);      
    else
      return make_shared<HDivHighOrderFESpace> (ma, flags, true);
  }

  
  void RaviartThomasFESpace :: Update()
  {
    shared_ptr<MeshAccess> ma = GetMeshAccess();
    int level = ma->GetNLevels();
    
    if (level == ndlevel.Size())
      return;
    
    if (ma->GetDimension() == 2)
      ndlevel.Append (ma->GetNEdges());
    else
      ndlevel.Append (ma->GetNFaces());

    // FinalizeUpdate (lh);
  }

  FiniteElement & RaviartThomasFESpace :: GetFE (ElementId ei, Allocator & lh) const
  {
    switch(ma->GetElType(ei))
      {
      case ET_TRIG: return *(new (lh) FE_RTTrig0);
      case ET_SEGM: return *(new (lh) HDivNormalSegm0);
      default: throw Exception ("Element type not available for RaviartThomasFESpace::GetFE");
      }
  }
  
  size_t RaviartThomasFESpace :: GetNDof () const throw()
  {
    return ndlevel.Last();
  }
  
  size_t RaviartThomasFESpace :: GetNDofLevel (int level) const
  {
    return ndlevel[level];
  }
  
  
  
  void RaviartThomasFESpace :: GetDofNrs (ElementId ei, Array<int> & dnums) const
  {
    if(ei.VB()==VOL)
      {
	Array<int> forient(6);
	
	if (ma->GetDimension() == 2)
	  ma->GetElEdges (ei.Nr(), dnums, forient);
	else
	  ma->GetElFaces (ei.Nr(), dnums, forient);
	
	if (!DefinedOn (ei))
	  dnums = -1;
        return;
      }

    if(ei.VB()==BND)
      {
	if (ma->GetDimension() == 2)
	  {
	    int eoa[12];
	    Array<int> eorient(12, eoa);
	    ma->GetSElEdges (ei.Nr(), dnums, eorient);
	    
	    if (!DefinedOn(ei))
	      dnums = -1;
            
          }
    // (*testout) << "el = " << elnr << ", dofs = " << dnums << endl;
      }
  
    if(ei.VB()==BBND || ei.VB()==BBBND)
      {
        dnums.SetSize0();
        return;
      }

    /*
      int eoa[12];
      Array<int> eorient(12, eoa);
      GetMeshAccess().GetSElEdges (selnr, dnums, eorient);
      
      if (!DefinedOnBoundary (ma->GetSElIndex (selnr)))
      dnums = -1;
    */
    dnums.SetSize (1);
    dnums = -1;
    
    // (*testout) << "sel = " << selnr << ", dofs = " << dnums << endl;
  }

  /*
  Table<int> * RaviartThomasFESpace :: CreateSmoothingBlocks (int type) const
  {
    return 0;
  }
  */
  
  void RaviartThomasFESpace :: 
  VTransformMR (ElementId ei, 
		SliceMatrix<double> mat, TRANSFORM_TYPE tt) const
  {
    int nd = 3;
    bool boundary = (ei.VB() == BND);
    size_t elnr = ei.Nr();
    if (boundary) return;

    Vector<> fac(nd);
    
    GetTransformationFactors (elnr, fac);
    
    int i, j, k, l;
    
    if (tt & TRANSFORM_MAT_LEFT)
      for (k = 0; k < dimension; k++)
	for (i = 0; i < nd; i++)
	  for (j = 0; j < mat.Width(); j++)
	    mat(k+i*dimension, j) *= fac(i);
	
    if (tt & TRANSFORM_MAT_RIGHT)
      for (l = 0; l < dimension; l++)
	for (i = 0; i < mat.Height(); i++)
	  for (j = 0; j < nd; j++)
	    mat(i, l+j*dimension) *= fac(j);
  }
  
    
  void  RaviartThomasFESpace ::
  VTransformVR (ElementId ei, 
		SliceVector<double> vec, TRANSFORM_TYPE tt) const
  {
    int nd = 3;
    bool boundary = (ei.VB() == BND);
    size_t elnr = ei.Nr();
    
    if (boundary) 
      {
	ArrayMem<int,4> edge_nums, edge_orient;
	ma->GetSElEdges (elnr, edge_nums, edge_orient);
	vec *= edge_orient[0];
	return;
      }

    Vector<> fac(nd);
    
    GetTransformationFactors (elnr, fac);
    
    if ((tt & TRANSFORM_RHS) || (tt & TRANSFORM_SOL) || (tt & TRANSFORM_SOL_INVERSE))
      {
	for (int k = 0; k < dimension; k++)
	  for (int i = 0; i < nd; i++)
	    vec(k+i*dimension) *= fac(i);
      }
  }
  
  void RaviartThomasFESpace ::
  GetTransformationFactors (int elnr, FlatVector<> & fac) const
  {
    Array<int> edge_nums, edge_orient;
    
    fac = 1;

    ma->GetElEdges (elnr, edge_nums, edge_orient);
    for (int i = 0; i < 3; i++)
      fac(i) = edge_orient[i];
  }



  
  class BDM1Prolongation : public Prolongation
  {
    shared_ptr<MeshAccess> ma;
    const FESpace & space;

    array<Mat<3,3>, 8> boundaryprol;
    
  public:
    BDM1Prolongation(const FESpace & aspace)
      : ma(aspace.GetMeshAccess()), space(aspace)
    {
      // calc prolongation matrices here;
      Matrix<> pointc = { { 0, 0 }, { 1, 0 }, { 0, 1 } };
      FE_ElementTransformation<2,2> trafoc(ET_TRIG, pointc);

      Matrix<> pointf = { { 0, 0 }, { 0.5, 0 }, { 0, 1 } };
      FE_ElementTransformation<2,2> trafof(ET_TRIG, pointf);

      for (int classnr = 0; classnr < 8; classnr++)
        {
          int vertsc[3] = { 1, 2, 3 };
          if (classnr & 1) Swap(vertsc[0], vertsc[1]);
          if (classnr & 2) Swap(vertsc[1], vertsc[2]);
          if (classnr & 4) Swap(vertsc[0], vertsc[1]);

          HDivHighOrderNormalTrig<TrigExtensionMonomial> felc(1);
          felc.SetVertexNumbers (vertsc);

          int vertsf[3] = { vertsc[0], vertsc[1], 4 };
          HDivHighOrderNormalTrig<TrigExtensionMonomial> felf(1);
          felf.SetVertexNumbers (vertsf);

          IntegrationRule ir(ET_TRIG, 2);
          Matrix<> massf(3,3), massfc(3,3);
          Matrix<> shapef(3,1), shapec(3,1);
          massf = 0; massfc = 0;
          
          for (IntegrationPoint ip : ir)
            {
              IntegrationPoint ipc(0.5*ip(0), ip(1));
              MappedIntegrationPoint<2,2> mipc(ipc, trafoc);
              MappedIntegrationPoint<2,2> mipf(ip, trafof);

              felc.CalcMappedShape (mipc, shapec);
              felf.CalcMappedShape (mipf, shapef);

              massf += ip.Weight() * shapef * Trans(shapef);
              massfc += ip.Weight() * shapef * Trans(shapec);
            }
          CalcInverse (massf);
          boundaryprol[classnr] = massf * massfc;
          cout << "boundarypol[" << classnr << "] = " << endl << FlatMatrix(boundaryprol[classnr]) << endl;
        }
      
    }

    
    virtual ~BDM1Prolongation() { }
  
    virtual void Update (const FESpace & fes) { ; }
    virtual SparseMatrix< double >* CreateProlongationMatrix( int finelevel ) const
    { return nullptr; }

    virtual void ProlongateInline (int finelevel, BaseVector & v) const
    {
      size_t nc = space.GetNDofLevel (finelevel-1) / 3;
      size_t nf = space.GetNDofLevel (finelevel) / 3;
      
      auto fv = v.FV<double>();
      fv.Range(3*nf, fv.Size()) = 0;

      for (size_t i = nc; i < nf; i++)
        {
          auto [info, nrs] = ma->GetParentFaces(i);
	  int pa1 = nrs[0];
	  int pa2 = nrs[1];
	  int pa3 = nrs[2];
          int pa4 = nrs[2];

          if (pa2 == -1)
            {
              Vec<3> fvecc = fv.Range(3*pa1, 3*pa1+3);
              Vec<3> fvecf = boundaryprol[info%8] * fvecc;
              fv.Range(3*i, 3*i+3) = fvecf;
            }
          else
            {
              // missing ...
              fv.Range(3*i, 3*i+3) = 0.0;
            }
        }

      /*
        // todo
      // every edge from coarse level got split
      for (size_t i = 0; i < nf; i++)
        {
          auto [info, nrs] = ma->GetParentEdges(i);
          if (nrs[0] != -1 && nrs[1] == -1)
            {
              fv(2*nrs[0]) = 0;
              fv(2*nrs[0]+1) = 0;
            }
        }
      */
    }
    
    virtual void RestrictInline (int finelevel, BaseVector & v) const
    {
      /*
      size_t nc = space.GetNDofLevel (finelevel-1) / 2;
      size_t nf = space.GetNDofLevel (finelevel) / 2;
      
      auto fv = v.FV<double>();
      fv.Range(2*nf, fv.Size()) = 0;

      // every edge from coarse level got split
      for (size_t i = 0; i < nf; i++)
        {
          auto [info, nrs] = ma->GetParentEdges(i);
          if (nrs[0] != -1 && nrs[1] == -1)
            {
              fv(2*nrs[0]) = 0;
              fv(2*nrs[0]+1) = 0;
            }
        }

      
      for (size_t i = nf; i-- > nc; )
        {
          auto [info, nrs] = ma->GetParentEdges(i);
	  int pa1 = nrs[0];
	  int pa2 = nrs[1];
	  int pa3 = nrs[2];

          if (pa2 == -1)
            {
              double fac0 = (info & 1) ? 0.5 : -0.5;
              fv(2*pa1) += fac0 * fv(2*i);
              fv(2*pa1+1) += -0.125 * fv(2*i) + 0.25 * fv(2*i+1);
            }
          else
            {
              double fac1 = (info&1) ? 0.5 : -0.5;
              double fac2 = (info&2) ? 0.5 : -0.5;
              double fac3 = (info&4) ? -0.125 : 0.125;
              fv(2*pa1)   += fac1 * fv(2*i);
              fv(2*pa1+1) += 0.5 * fv(2*i+1);
              fv(2*pa2)   += fac2 * fv(2*i);
              fv(2*pa2+1) += 0.5 * fv(2*i+1);
              fv(2*pa3+1) += fac3 * fv(2*i) - 0.25 * fv(2*i+1);
            }
        }
      */
    }
  };

  

  class NGS_DLL_HEADER BDM1FESpace : public FESpace
  {
    BitArray active_facets;
    
  public:
    BDM1FESpace (shared_ptr<MeshAccess> ama, const Flags & flags, bool parseflags=false)
      : FESpace(ama, flags)
      {
        name="BDM1FESpace";
        
        if (ma->GetDimension() == 2)
          {
            evaluator[VOL] = make_shared<T_DifferentialOperator<DiffOpIdHDiv<2>>>();
            evaluator[BND] = make_shared<T_DifferentialOperator<DiffOpIdVecHDivBoundary<2>>>();
            // additional_evaluators.Set ("grad", make_shared<T_DifferentialOperator<DiffOpGradientHDiv<2>>> ());
          }
        else if(ma->GetDimension() == 3) 
          {
            evaluator[VOL] = make_shared<T_DifferentialOperator<DiffOpIdHDiv<3>>>();
            evaluator[BND] = make_shared<T_DifferentialOperator<DiffOpIdVecHDivBoundary<3>>>();

            // additional_evaluators.Set ("grad", make_shared<T_DifferentialOperator<DiffOpGradientHCurl<3>>> ());            
          }
        prol = make_shared<BDM1Prolongation> (*this);
      }
    
    virtual ~BDM1FESpace () { }
    virtual const char * GetType()  { return "BDM1"; }
    
    static shared_ptr<FESpace> Create (shared_ptr<MeshAccess> ma, const Flags & flags)
    {
      return make_shared<BDM1FESpace> (ma, flags, true);
    }
    
    void Update() override
    {
      if (ma->GetDimension() == 3)
        {
          size_t nfa = ma->GetNFaces();
          SetNDof (3*nfa);
          active_facets = BitArray(nfa);
          active_facets.Clear();
          for (auto el : ma->Elements(VOL))
            for (auto fa : el.Faces())
              active_facets.SetBit(fa);
      
          ctofdof.SetSize(GetNDof());
          ctofdof = WIREBASKET_DOF;
          for (size_t i = 0; i < nfa; i++)
            if (!active_facets.Test(i))
              ctofdof[3*i] = ctofdof[3*i+1] = ctofdof[3*i+2] = UNUSED_DOF;
          // cout << "active edges = " << endl << active_edges << endl;
        }
    }
    
    // virtual void DoArchive (Archive & archive) override;
    // virtual void UpdateCouplingDofArray() override;
    
    virtual FiniteElement & GetFE (ElementId ei, Allocator & lh) const override
    {
      if (ei.VB() == VOL)
        switch (ma->GetElType(ei))
          {
          case ET_TET:   
            {
              Ngs_Element ngel = ma->GetElement<3,VOL> (ei.Nr());
              if (!DefinedOn(ngel)) return * new (lh) HDivDummyFE<ET_TET>();
    
              auto * fe =  new (lh) HDivHighOrderFE<ET_TET> (1);
              fe -> SetVertexNumbers (ngel.Vertices());
              return *fe;
            }

          case ET_TRIG:   
          default:
            throw Exception ("Inconsistent element type in NedelecFESpace::GetFE");
          }
      
      else if (ei.VB() == BND)

        switch (ma->GetElType(ei))
          {
          case ET_TRIG:   
            {
              Ngs_Element ngel = ma->GetElement<2,BND> (ei.Nr());
              if (!DefinedOn(ngel)) return * new (lh) HDivNormalDummyFE<ET_TRIG>();
    
              auto fe = new (lh) HDivHighOrderNormalTrig<TrigExtensionMonomial> (1);
              fe -> SetVertexNumbers (ngel.Vertices());
              return *fe;
            }
          default:
            throw Exception ("Inconsistent element type in NedelecFESpace::GetFE");
          }
      
    }
    
    virtual void GetDofNrs (ElementId ei, Array<DofId> & dnums) const override
    {
      if (ma->GetDimension() == 3)
        {
          auto faces = ma->GetElFaces (ei);
          dnums.SetSize(3*faces.Size());
          for (int i : Range(faces))
            {
              dnums[i] = 3*faces[i];
              dnums[2*i+faces.Size()] = 3*faces[i]+1;
              dnums[2*i+1+faces.Size()] = 3*faces[i]+2;
            }
        }
    }

    
    virtual string GetClassName () const override
    {
      return "BDM11FESpace";
    }
    
    virtual void GetVertexDofNrs (int vnr, Array<DofId> & dnums) const override
    { dnums.SetSize0(); }
    virtual void GetEdgeDofNrs (int ednr, Array<DofId> & dnums) const override
    {
      dnums.SetSize0();
      /*
        // for 2D ...
      if (active_edges.Test(ednr))
        {
          dnums.SetSize(2);
          dnums[0] = 2*ednr;
          dnums[1] = 2*ednr+1;
        }
      else
        dnums.SetSize0();
      */
    }
    virtual void GetFaceDofNrs (int fanr, Array<DofId> & dnums) const override
    {
      if (active_facets.Test(fanr))
        {
          dnums.SetSize(3);
          dnums[0] = 3*fanr;
          dnums[1] = 3*fanr+1;
          dnums[2] = 3*fanr+2;
        }
      else
        dnums.SetSize0();
    }    
    virtual void GetInnerDofNrs (int elnr, Array<DofId> & dnums) const override
    { dnums.SetSize0(); }    
  };

  static RegisterFESpace<BDM1FESpace> initbdm1 ("BDM1");




  

  // register FESpaces
  namespace hdivfes_cpp
  {
    class Init
    { 
    public: 
      Init ();
    };
    
    Init::Init()
    {
      GetFESpaceClasses().AddFESpace ("hdiv", RaviartThomasFESpace::Create,
                                      RaviartThomasFESpace::GetDocu);
    }
    
    Init init;
  }
}
