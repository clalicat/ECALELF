#include <RooStats/MCMCIntervalPlot.h>
#include <TFile.h>
#include <TTree.h>
#include <TString.h>
#include <TObjArray.h>
#include <TROOT.h>
#include <TMatrixDSym.h>
#include <TCanvas.h>
#include <RooMultiVarGaussian.h>

#include <TCut.h>
#include <TEntryList.h>
#include <TH2F.h>
#include <TH1F.h>

#include <RooRealVar.h>
#include <RooArgSet.h>
#include <RooDataHist.h>
#include <RooHistPdf.h>
#include <TGraphErrors.h>
#include <TMultiGraph.h>

#include <iostream>
#include <TFile.h>
#include <TString.h>

#include <TH1F.h>
#include <TH2F.h>
#include <TH2D.h>
#include <map>
#include <TCanvas.h>
#include <TLegend.h>
#include <TPaveText.h>
#include <TStyle.h>
#include <TROOT.h>
#include <TList.h>
#include <TObject.h>
#include <TKey.h>
#include <TPaveText.h>
#include <TLine.h>
#include <TGaxis.h>
#include <iomanip> 
#include <TList.h>

TGraph* bestFit(TTree *t, TString x, TString y, TString nll) {
    t->Draw(y+":"+x, nll+" == 0");
    TGraph *gr0 = (TGraph*) gROOT->FindObject("Graph")->Clone();
    gr0->SetMarkerStyle(34); gr0->SetMarkerSize(2.0);
    if (gr0->GetN() > 1) gr0->Set(1);
    return gr0;
}

TH2D* frameTH2D(TH2D *in){

	Double_t xw = in->GetXaxis()->GetBinWidth(0);
	Double_t yw = in->GetYaxis()->GetBinWidth(0);

	Int_t nx = in->GetNbinsX();
	Int_t ny = in->GetNbinsY();

	Double_t x0 = in->GetXaxis()->GetXmin();
	Double_t x1 = in->GetXaxis()->GetXmax();

	Double_t y0 = in->GetYaxis()->GetXmin();
	Double_t y1 = in->GetYaxis()->GetXmax();

	TH2D *framed = new TH2D(
			Form("%s framed",in->GetName()),
			Form("%s framed",in->GetTitle()),
			nx + 2, x0-xw, x1+xw,
			ny + 2, y0-yw, y1+yw
			);

	//Copy over the contents
	for(int ix = 1; ix <= nx ; ix++){
		for(int iy = 1; iy <= ny ; iy++){
			framed->SetBinContent(1+ix, 1+iy, in->GetBinContent(ix,iy));
		}
	}
	//Frame with huge values
	nx = framed->GetNbinsX();
	ny = framed->GetNbinsY();
	for(int ix = 1; ix <= nx ; ix++){
		framed->SetBinContent(ix,  1, 1000.);
		framed->SetBinContent(ix, ny, 1000.);
	}
	for(int iy = 2; iy <= ny-1 ; iy++){
		framed->SetBinContent( 1, iy, 1000.);
		framed->SetBinContent(nx, iy, 1000.);
	}

	return framed;
}


TList* contourFromTH2(TH2 *h2in, double threshold) {
    std::cout << "Getting contour at threshold " << threshold << " from " << h2in->GetName() << std::endl;
    //http://root.cern.ch/root/html/tutorials/hist/ContourList.C.html
    Double_t contours[1];
    contours[0] = threshold;

    TH2D *h2 = frameTH2D((TH2D*)h2in);

    h2->SetContour(1, contours);

    // Draw contours as filled regions, and Save points
    h2->Draw("CONT Z LIST");
    gPad->Update(); // Needed to force the plotting and retrieve the contours in TGraphs

    // Get Contours
    TObjArray *conts = (TObjArray*)gROOT->GetListOfSpecials()->FindObject("contours");
    TList* contLevel = NULL;

    if (conts == NULL || conts->GetSize() == 0){
        printf("*** No Contours Were Extracted!\n");
        return 0;
    }

    TList *ret = new TList();
    for(int i = 0; i < conts->GetSize(); i++){
        contLevel = (TList*)conts->At(i);
        printf("Contour %d has %d Graphs\n", i, contLevel->GetSize());
        for (int j = 0, n = contLevel->GetSize(); j < n; ++j) {
            TGraph *gr1 = (TGraph*) contLevel->At(j)->Clone();
            ret->Add(gr1);
        }
    }
    return ret;
}


TGraphErrors g(TTree *genTree, TString constTermName=""){
  Double_t alpha, constTerm;
  genTree->SetBranchAddress("alpha",&alpha);
  genTree->SetBranchAddress("constTerm",&constTerm);

  Long64_t nEntries=genTree->GetEntries();

  TH1F h("smearingHist","",10000,0,0.1);
  TGraphErrors graph;
  Int_t iPoint=0;
  for(Double_t energy=20; energy<150; energy+=10){
    h.Reset();
    for(Long64_t jentry=0; jentry<nEntries; jentry++){
      genTree->GetEntry(jentry);
      h.Fill(sqrt(alpha*alpha/energy+constTerm*constTerm));
    }
    graph.SetPoint(iPoint, energy, h.GetMean());
    graph.SetPointError(iPoint,0, h.GetRMS());
    iPoint++;
  }
  h.SetTitle(constTermName);
  h.SaveAs("tmp/h-"+constTermName+".root");
  graph.Set(iPoint);
  genTree->ResetBranchAddresses();
  graph.Draw("A L3");
  graph.SetFillColor(kBlue);
  graph.SetLineColor(kYellow);
  graph.GetXaxis()->SetTitle("Energy [GeV]");
  graph.GetYaxis()->SetTitle("Additional smearing [%]");
  return graph;
}

TGraphErrors g(TTree *tree, TString alphaName, TString constTermName){
  Double_t alpha, constTerm;
  alphaName.ReplaceAll("-","_");
  constTermName.ReplaceAll("-","_");

  tree->SetBranchAddress(alphaName,&alpha);
  tree->SetBranchAddress(constTermName,&constTerm);

  //Long64_t nEntries=genTree->GetEntries();

  TGraphErrors graph;
  Int_t iPoint=0;

  tree->GetEntry(0);
  std::cout << alpha << "\t" << constTerm << std::endl;
  Double_t alpha2=alpha*alpha;
  Double_t const2=constTerm*constTerm;
  for(Double_t energy=20; energy<150; energy+=10){
    Double_t addSmearing = (sqrt(alpha2/energy+const2));
    
    graph.SetPoint(iPoint, energy, addSmearing);
    graph.SetPointError(iPoint,0, 0);
    iPoint++;
  }

  graph.Set(iPoint);
  tree->ResetBranchAddresses();
  graph.Draw("A L");
  graph.SetFillColor(kBlue);
  graph.SetLineColor(kYellow);
  graph.GetXaxis()->SetTitle("Energy [GeV]");
  graph.GetYaxis()->SetTitle("Additional smearing [%]");
  return graph;
}

TGraphErrors g(Double_t alpha, Double_t constTerm){
  TGraphErrors graph;
  Int_t iPoint=0;

  //  std::cout << alpha << "\t" << constTerm << std::endl;
  Double_t alpha2=alpha*alpha;
  Double_t const2=constTerm*constTerm;
  for(Double_t energy=20; energy<150; energy+=10){
    Double_t addSmearing = (sqrt(alpha2/energy+const2));
    
    graph.SetPoint(iPoint, energy, addSmearing);
    graph.SetPointError(iPoint,0, 0);
    iPoint++;
  }

  graph.Set(iPoint);
  graph.Draw("A L");
  graph.SetFillColor(kBlue);
  graph.SetLineColor(kYellow);
  graph.GetXaxis()->SetTitle("Energy [GeV]");
  graph.GetYaxis()->SetTitle("Additional smearing [%]");
  return graph;
}

TGraphErrors bestFit(TTree *tree, TString alphaName, TString constTermName){
  Double_t alpha, constTerm;
  alphaName.ReplaceAll("-","_");
  constTermName.ReplaceAll("-","_");

  tree->SetBranchAddress(alphaName,&alpha);
  tree->SetBranchAddress(constTermName,&constTerm);

  //Long64_t nEntries=genTree->GetEntries();

  TGraphErrors graph;
  Int_t iPoint=0;

  tree->GetEntry(0);
  graph.SetPoint(0, constTerm, alpha);
  graph.SetPointError(0,0, 0);
  iPoint++;

  graph.Set(iPoint);
  tree->ResetBranchAddresses();
  graph.Draw("A P");
//   graph.SetFillColor(kBlue);
//   graph.SetLineColor(kYellow);
//   graph.GetXaxis()->SetTitle("Energy [GeV]");
//   graph.GetYaxis()->SetTitle("Additional smearing [%]");
  return graph;
}

RooHistPdf *nllToL(TH2F* hist){
  TH2F *h = (TH2F*) hist->Clone(TString(hist->GetName())+"_L");

  h->Reset();
  Double_t min=1e20, max=0;
  for(Int_t iBinX=1; iBinX <= hist->GetNbinsX(); iBinX++){
      for(Int_t iBinY=1; iBinY <= hist->GetNbinsY(); iBinY++){
	Double_t binContent=hist->GetBinContent(iBinX, iBinY);
	if(min>binContent && binContent!=0) min=binContent;
	if(max<binContent) max=binContent;
      }
  }

  for(Int_t iBinX=1; iBinX <= hist->GetNbinsX(); iBinX++){
    for(Int_t iBinY=1; iBinY <= hist->GetNbinsY(); iBinY++){
      Double_t binContent=hist->GetBinContent(iBinX, iBinY);
      Double_t b = binContent <= 0 ? 0 : exp(-binContent+min+50);
      if(binContent != 0 && binContent-min<100) std::cout << iBinX << "\t" << iBinY << "\t" << binContent << "\t" << -binContent+min << "\t" << b << std::endl;
      //h->Fill(hist->GetXaxis()->GetBinLowEdge(iBinX), hist->GetYaxis()->GetBinLowEdge(iBinY),b);
      h->SetBinContent(iBinX, iBinY,b);
    }
  }
  
  
  RooRealVar *var1 = new RooRealVar("constTerm","",0.1);
  RooRealVar *var2 = new RooRealVar("alpha","",0.1);
  
  RooDataHist *dataHist = new RooDataHist(TString(hist->GetName())+"_dataHist",hist->GetTitle(), RooArgSet(*var1,*var2), h);
  RooHistPdf *histPdf = new RooHistPdf(TString(hist->GetName())+"_histPdf",hist->GetTitle(), RooArgSet(*var1,*var2), *dataHist);

  //delete dataHist;
  return histPdf;
}

TH2F *prof2d(TTree *tree, TString var1Name, TString var2Name, TString nllName, TString binning="(40,0,0.05,40,0,0.2)", bool delta=false){

  var1Name.ReplaceAll("-","_");
  var2Name.ReplaceAll("-","_");
  //  tree->Print();
  TString histName="h";
  std::cout << var1Name << "\t" << var2Name << "\t" << histName << std::endl;
  tree->Draw(var1Name+":"+var2Name+">>"+histName+binning);
  TH2F *hEntries = (TH2F*)gROOT->FindObject(histName);
  if(hEntries==NULL) return NULL;
  //std::cerr << "e qui ci sono?" << std::endl;
  tree->Draw(var1Name+":"+var2Name+">>shervin"+binning,nllName);

  TH2F *h = (TH2F*)gROOT->FindObject("shervin");
  if(h==NULL) return NULL;
  h->Divide(hEntries);


  //std::cerr << "io sono qui" << std::endl;    
  delete hEntries;
  Double_t min=1e20, max=0;

  if(delta){
    for(Int_t iBinX=1; iBinX <= h->GetNbinsX(); iBinX++){
      for(Int_t iBinY=1; iBinY <= h->GetNbinsY(); iBinY++){
	Double_t binContent=h->GetBinContent(iBinX, iBinY);
	if(min>binContent && binContent!=0) min=binContent;
	if(max<binContent) max=binContent;
      }
    }
    std::cout << "min=" << min << "\tmax=" << max<<std::endl;    
    for(Int_t iBinX=1; iBinX <= h->GetNbinsX(); iBinX++){
      for(Int_t iBinY=1; iBinY <= h->GetNbinsY(); iBinY++){
	Double_t binContent=h->GetBinContent(iBinX, iBinY);
	//std::cout << binContent << std::endl;
	if(binContent!=0) binContent-=min;
	else binContent=-1;
	h->SetBinContent(iBinX,iBinY,binContent);
      }
    }
  }
  h->GetZaxis()->SetRangeUser(0,500);
  //std::cerr << "io sono qui 3" << std::endl;    
  return h;
//   Double_t variables[2];
//   Double_t nll;
//   tree->SetBranchAddress(var1Name, &(variables[0]));
//   tree->SetBranchAddress(var2Name, &(variables[1]));
//   tree->SetBranchAddress(nllName, &(nll));

  
//   Long64_t nEntries=tree->GetEntries();
//   for(Long64_t jentry=0; jentry<nEntries; jentry++){
    
}


// TMatrixDSym* GetCovariance( RooStats::MarkovChain *chain, TString var1, TString var2){

//   RooRealVar *nll = chain->GetNLLVar();
//   RooRealVar *weight= chain->GetWeightVar();
//   RooFormulaVar newWeight("f","","-@0",nll);
  
//   RooArgSet *args = chain->Get();
//   RooArgSet newArg;
//   RooArgSet oldArg;
//   oldArg.add(*(args->find(var1)));
//   oldArg.add(*(args->find(var2)));
//   oldArg.add(*nll);
//   newArg.add(*(args->find(var1)));
//   newArg.add(*(args->find(var2)));
//   //newArg.add(*nll);
//   //newArg.add(newWeight);

//   RooDataSet dataset("dataset","",   chain->GetAsDataSet(oldArg), RooArgSet(oldArg,newWeight), "", newWeight->GetName());
//   return dataset->covarianceMatrix(newArg);
// }

//------------------------------
TCut m(TTree *tree, TString var1, TString var2){
  Double_t min= tree->GetMinimum("nll_MarkovChain_local_");
  TString selMin="nll_MarkovChain_local_ == "; selMin+=min;
  
  tree->Draw(">>entrylist",selMin,"entrylist");
  TEntryList *entrylist=(TEntryList*) gROOT->FindObject("entrylist");
  if(entrylist->GetN()!=1){
    std::cout << "Entrylist N=" << entrylist->GetN() << std::endl;
    return "";
  }
  Long64_t minEntry = entrylist->GetEntry(0);

  TObjArray *branchArray = tree->GetListOfBranches();
  branchArray->GetEntries();

  std::vector<TString> minSel_vec;

  for(int i=0; i < branchArray->GetEntriesFast(); i++){
    TBranch *b= (TBranch *) branchArray->At(i);
    TString name=b->GetName();
    Double_t address;
    b->SetAddress(&address);
    b->GetEntry(minEntry);
    name+="==";name+=address;
    minSel_vec.push_back(name);
  }

  TCut cut;
  for(int i=0; i < branchArray->GetEntriesFast(); i++){
    TBranch *b= (TBranch *) branchArray->At(i);
    TString name=b->GetName();
    if(name.Contains("nll")) continue;
    if(name.Contains(var1)) continue;
    if(name.Contains(var2)) continue;
    cut+=minSel_vec[i];
  }
  return cut;
}

RooMultiVarGaussian *MultiVarGaussian(RooDataSet *dataset){

  RooArgSet *args = (RooArgSet *) dataset->get()->Clone();
  //   argSet.remove(*chain->GetNLLVar(), kFALSE, kTRUE);

  RooArgSet *mu = (RooArgSet *)args->Clone();
  TMatrixDSym *matrix = dataset->covarianceMatrix();
  RooMultiVarGaussian *g = new RooMultiVarGaussian("multi","",RooArgList(*args), RooArgList(*mu),*matrix);
  return g;
  
  

  return NULL;
}

TTree *dataset2tree(RooDataSet *dataset){

  const RooArgSet *args = dataset->get();
  RooArgList argList(*args);

  Double_t variables[50];
  Long64_t nEntries= dataset->numEntries();
  //nEntries=1;
  TTree *tree = new TTree("tree","tree");
  tree->SetDirectory(0);
  TIterator *it1=NULL; 
  it1 = argList.createIterator();
  int index1=0;
  for(RooRealVar *var = (RooRealVar *) it1->Next(); var!=NULL;
      var = (RooRealVar *) it1->Next(),index1++){
    TString name(var->GetName());
    name.ReplaceAll("-","_");
    tree->Branch(name, &(variables[index1]), name+"/D");
  }

  //  tree->Print();

  for(Long64_t jentry=0; jentry<nEntries; jentry++){
    
    TIterator *it=NULL; 
    RooArgList argList1(*(dataset->get(jentry)));
    it = argList1.createIterator();
    //(dataset->get(jentry))->Print();
    int index=0;
    for(RooRealVar *var = (RooRealVar *) it->Next(); var!=NULL;
	var = (RooRealVar *) it->Next(), index++){
      variables[index]=var->getVal();
      //var->Print();
    }
   
    delete it;
    tree->Fill();
  }
  tree->ResetBranchAddresses();
  //  tree->Print();
  return tree;
}

void mcmcDraw(TFile *f){
  RooStats::MarkovChain *chain = (RooStats::MarkovChain *)f->Get("_markov_chain");
  RooStats::MCMCInterval mcInter;
  mcInter.SetChain(*chain);
  RooStats::MCMCIntervalPlot mcInterPlot(mcInter);
  mcInterPlot.DrawNLLHist();
  mcInterPlot.DrawNLLVsTime();
  TCanvas *c = (TCanvas *)gROOT->FindObject("c1");
  c -> SaveAs("tmp/NLLVsTime.png");
  c->Clear();
  mcInterPlot.Draw();
  c->SaveAs("tmp/l.png");

//   TTree *tree = (TTree *)chain->GetAsDataSet()->tree();
//   RooArgSet *args = (RooArgSet *) chain->Get()->Clone();
//   RooArgSet argSet(*chain->Get());
//   argSet.remove(*chain->GetNLLVar(), kFALSE, kTRUE);
  
//   TObjArray *branchList = tree->GetListOfBranches();
//   std::set<TString> branchNames;
//   for(int i=0; i < branchList->GetSize(); i++){
//     if(branchList->At(i)!=NULL){
//       TString bName(branchList->At(i)->GetName());
//       if(bName != chain->GetNLLVar()->GetName() &&
// 	 bName != chain->GetWeightVar()->GetName())
// 	branchNames.insert(bName);
//     }
//   }


    
//   for(std::set<TString>::const_iterator b_itr = branchNames.begin();
//       b_itr != branchNames.end();
//       b_itr++){
//     std::cout << *b_itr << std::endl;
//     //argSet.add(*(args->find(*b_itr)));
//     tree->Draw(*b_itr+">>h",TString(chain->GetWeightVar()->GetName()));
//     TH1F *h = (TH1F *) gROOT->FindObject("h");
//     TString cut=TString::Format("(100,%.2f,%.2f)",h->GetMean()-3*h->GetRMS() ,h->GetMean()+3*h->GetRMS());
//     delete h;
//     tree->Draw(*b_itr+">>h"+cut,TString(chain->GetWeightVar()->GetName()));
//     h = (TH1F *) gROOT->FindObject("h");
    
//     //((RooRealVar *)(args->find(*b_itr)))->setVal(
//     std::cout << *b_itr << "\t" << h->GetMean() << "\t" << h->GetRMS() << std::endl;;
//     h->Draw();
//     h->Fit("gaus");
//     c->SaveAs("tmp/"+*b_itr+".png");
//     h->SaveAs("tmp/"+*b_itr+".root");
//     //    args->find(*b_itr)
//     delete h;
//   }
//   argSet.Print();
  
//   argSet.writeToStream(std::cout, kFALSE);
  
//   TMatrixDSym *matrix = chain->GetAsDataSet()->covarianceMatrix(argSet);
//   matrix->SaveAs("tmp/matrix.root");
  
}

TGraphErrors plot(TFile *f, TString alpha, TString constTerm){
  RooStats::MarkovChain *chain = (RooStats::MarkovChain *)f->Get("_markov_chain");
  TTree *tree = dataset2tree(chain->GetAsDataSet());
  TH2F *hist = prof2d(tree, alpha, constTerm, "nll_MarkovChain_local_");
  RooHistPdf *histPdf = nllToL(hist);
  TTree *genTree = dataset2tree(histPdf->generate(*histPdf->getVariables(),1000000,kTRUE,kFALSE));
  TGraphErrors graph = g(genTree);
  
  delete tree;
  delete hist;
  delete histPdf;
  delete genTree;

  return graph;
  
}

TGraphErrors plot(RooDataSet *dataset, TString alpha, TString constTerm){
  TTree *tree = dataset2tree(dataset);
  TH2F *hist = prof2d(tree, alpha, constTerm, "nll");
  RooHistPdf *histPdf = nllToL(hist);
  TTree *genTree = dataset2tree(histPdf->generate(*histPdf->getVariables(),1000000,kTRUE,kFALSE));
  TGraphErrors graph = g(genTree);
  
  delete tree;
  delete hist;
  delete histPdf;
  delete genTree;

  return graph;
  
}




void MakePlots(TString filename, TString energy="8TeV", TString lumi=""){
  TString outDir=filename; outDir.ReplaceAll("fitres","img");
  outDir="tmp/";
  //std::map<TString, TH2F *> deltaNLL_map;
  
  /*------------------------------ Plotto */
  TCanvas *c = new TCanvas("c","c");
  
  TFile f_in(filename, "read");
  if(f_in.IsZombie()){
    std::cerr << "File opening error: " << filename << std::endl;
    return;
  }
  
  TList *KeyList = f_in.GetListOfKeys();
  std::cout << KeyList->GetEntries() << std::endl;
  for(int i =0; i <  KeyList->GetEntries(); i++){
    c->Clear();
    TKey *key = (TKey *)KeyList->At(i);
    if(TString(key->GetClassName())!="RooDataSet") continue;
    RooDataSet *dataset = (RooDataSet *) key->ReadObj();
    
    TString constTermName = dataset->GetName();
    TString alphaName=constTermName; alphaName.ReplaceAll("constTerm","alpha");

    TTree *tree = dataset2tree(dataset);
    TGraphErrors bestFit_ = bestFit(tree, alphaName, constTermName);
    TH2F *hist = prof2d(tree, alphaName, constTermName, "nll", "(12,-0.0005,0.0115,29,-0.0025,0.1425)",true);

//     //    deltaNLL_map.insert(std::pair <TString, TH2F *>(keyName,hist));
    hist->SaveAs(outDir+"/deltaNLL-"+constTermName+".root");
    hist->Draw("colz");
    bestFit_.Draw("P same");
    bestFit_.SetMarkerSize(2);

    Int_t iBinX, iBinY;
    Double_t x,y;
    hist->GetBinWithContent2(0,iBinX,iBinY);
    x= hist->GetXaxis()->GetBinCenter(iBinX);
    y= hist->GetYaxis()->GetBinCenter(iBinY);
    TGraph nllBestFit(1,&x,&y);

    nllBestFit.SetMarkerStyle(3);
    nllBestFit.SetMarkerColor(kRed);
    TList* contour68 = contourFromTH2(hist, 0.68);

    hist->Draw("colz");
    hist->GetZaxis()->SetRangeUser(0,50);
    bestFit_.Draw("P same");
    nllBestFit.Draw("P same");
    //contour68->Draw("same");
    c->SaveAs(outDir+"/deltaNLL-"+constTermName+".png");
    hist->SaveAs("tmp/hist-"+constTermName+".root");
    nllBestFit.SaveAs("tmp/nllBestFit.root");
    contour68->SaveAs("tmp/contour68.root");
    delete hist;
    hist = prof2d(tree, alphaName, constTermName, "nll", "(12,-0.0005,0.0115,29,-0.0025,0.1425)");
    RooHistPdf *histPdf = nllToL(hist);
    delete hist;
    RooDataSet *gen_dataset=histPdf->generate(*histPdf->getVariables(),1000000,kTRUE,kFALSE);
    TTree *genTree = dataset2tree(gen_dataset);
    genTree->SaveAs("tmp/genTree-"+constTermName+".root");
    delete gen_dataset;
    delete histPdf;
    
    TGraphErrors toyGraph = g(genTree, constTermName);
    TGraphErrors bestFitGraph = g(tree,alphaName, constTermName);
    TGraphErrors bestFitScanGraph = g(y, x);
    delete genTree;
    delete tree;
    toyGraph.SetFillColor(kGreen);
    toyGraph.SetLineColor(kBlue);
    toyGraph.SetLineStyle(2);
    bestFitGraph.SetLineColor(kBlack);
    bestFitScanGraph.SetLineColor(kRed);
    bestFitScanGraph.SetLineWidth(2);


    
    TMultiGraph g_multi("multigraph","");
    g_multi.Add(&toyGraph,"L3");
    g_multi.Add(&toyGraph,"L");
    g_multi.Add(&bestFitGraph, "L");
    g_multi.Add(&bestFitScanGraph, "L");
   
    g_multi.Draw("A");

    c->Clear();
    g_multi.Draw("A");
    c->SaveAs(outDir+"/smearing_vs_energy-"+constTermName+".png");

    //    TPaveText *pv = new TPaveText(0.7,0.7,1, 0.8);    
//     TLegend *legend = new TLegend(0.7,0.8,0.95,0.92);
//     legend->SetFillStyle(3001);
//     legend->SetFillColor(1);
//     legend->SetTextFont(22); // 132
//     legend->SetTextSize(0.04); // l'ho preso mettendo i punti con l'editor e poi ho ricavato il valore con il metodo GetTextSize()
//   //  legend->SetFillColor(0); // colore di riempimento bianco
//     legend->SetMargin(0.4);  // percentuale della larghezza del simbolo
    //    SetLegendStyle(legend);
	
    //Plot(c, data,mc,mcSmeared,legend, region, filename, energy, lumi);
  }
  
  f_in.Close();
  
  return;
}

// lowEtaBad
//tree->Draw(constTermName+":"+alphaName+">>h(16,0.0,0.08,20,0,0.02)",nllVarName+"-1.170791e+07","colz",10442-10232,10232) 

//highEtaBad
//tree->Draw(constTermName+":"+alphaName+">>h(16,0.0,0.08,20,0,0.02)",nllVarName+"-1.170791e+07","colz",11430-11061,11061) 

//lowEtaGold
//tree->Draw(constTermName+":"+alphaName+">>h(16,0.0,0.08,20,0,0.02)",nllVarName+"-1.170791e+07-4","colz",10226-9995,9995)
