// HyperRogue paper model generator
// Copyright (C) 2011-2018 Zeno Rogue, see 'hyper.cpp' for details

#if CAP_MODEL
namespace hr { namespace netgen {

  // We need a two-dimensional vector class for this.
  
  struct vec {
    double x, y;
    vec(double _x, double _y) : x(_x), y(_y) { }
    vec() : x(0), y(0) {}
    };
  
  vec& operator += (vec& a, const vec b) { a.x += b.x; a.y += b.y; return a; }
  vec& operator -= (vec& a, const vec b) { a.x -= b.x; a.y -= b.y; return a; }
  
  // coordinatewise multiplication and division
  vec& operator *= (vec& a, const vec b) { a.x *= b.x; a.y *= b.y; return a; }
  vec& operator *= (vec& a, double scalar) { a.x *= scalar; a.y *= scalar; return a; }
  vec& operator /= (vec& a, const vec b) { a.x /= b.x; a.y /= b.y; return a; }
  vec& operator /= (vec& a, double scalar) { a.x /= scalar; a.y /= scalar; return a; }
  
  vec  operator +  (vec a, const vec b) { return a+=b; }
  vec  operator -  (vec a, const vec b) { return a-=b; }
  vec  operator *  (vec a, const vec b) { return a*=b; }
  vec  operator /  (vec a, const vec b) { return a/=b; }
  
  vec  operator *  (vec a, double scalar)   { return a*=scalar; }
  vec  operator *  (double scalar, vec a)   { return a*=scalar; }
  vec  operator /  (vec a, double scalar)   { return a/=scalar; }
  vec  operator /  (double scalar, vec a)   { return a/=scalar; }

  vec ang(double f) { return vec(cos(f), sin(f)); }

  double norm(vec v) { return v.x*v.x+v.y*v.y; }

  // the parameters.
  
  bool loaded;
  
  int SCALE, PX, PY, BASE, SX, SY, CELLS, fontsize, created;
  double el;
  #define MAXCELLS 1000
  
  // All the datatables stored in the net files.
  
  int ct[MAXCELLS];
  double vx[MAXCELLS][16];
  vec center[MAXCELLS];
  double rot[MAXCELLS];  
  int glued[MAXCELLS];
  int nei[MAXCELLS][MAX_EDGE];

  // auxiliary data
  double raylen[MAXCELLS];
  double edgist[MAXCELLS];  
  char patek[MAXCELLS][MAX_EDGE];
  
  // data generated by HyperRogue
  hyperpoint hcenter[MAXCELLS][MAX_EDGE+1];
  
  // Functions handling the data.
  //==============================
  
  // Use HyperRogue to generate the data (ct, vx, nei).

  int mode = 0;
  
  void buildVertexInfo(cell *c, transmatrix V) {
    
    if(mode == 1)
    for(int ii=0; ii<CELLS; ii++) if(dcal[ii] == c) {
    
      hcenter[ii][MAX_EDGE] = V * C0;

      if(c->type == S7) {
        for(int i=0; i<c->type; i++) {
  
          int hdir = displayspin(c, i) + M_PI / S7;
        
          transmatrix V2 = V * spin(hdir) * xpush(hexf);
          
          hcenter[ii][i] = V2 * C0;
          }
        }
  
      if(c->type == S6) {
        for(int i=0; i<c->type; i++) {
  
          int hdir = displayspin(c, i);
        
          transmatrix V2 = 
            V * spin(hdir) * xpush(crossf) * spin(M_PI+M_PI/S7) * xpush(hexf);
          
          hcenter[ii][i] = V2 * C0;
          }
        }
      
      }
    }

  void dataFromHR() {
    mode = 1;
    drawthemap();
    mode = 0;

    for(int i=0; i<CELLS; i++) {
      ct[i] = dcal[i]->type;
      for(int k=0; k<8; k++)
        vx[i][2*k] = hcenter[i][k][0],
        vx[i][2*k+1] = hcenter[i][k][1];
      
      for(int k=0; k<ct[i]; k++) nei[i][k] = -1;

      for(int j=0; j<CELLS; j++) {
        cell *c1 = dcal[i];
        cell *c2 = dcal[j];
        for(int k=0; k<c1->type; k++) if(c1->move(k) == c2)
          nei[i][k] = j;
        }
      }

    for(int i=0; i<CELLS; i++) {
      center[i] = vec(SX/2, SY/2);
      rot[i] = 0;
      glued[i] = -1;
      for(int e=0; e<ct[i]; e++)
        if(nei[i][e] < i && nei[i][e] != -1 && (glued[i] == -1 || nei[i][e] < glued[i])) {
          glued[i] = nei[i][e];
          }
      }
    }
  
  void loadData() {

    FILE *f = fopen("papermodeldata.txt", "rt");
    if(!f) return;

    int err = fscanf(f, "%d %d %d %d %d %d %d %lf %d\n\n", 
      &CELLS, &SX, &SY, &PX, &PY, &SCALE, &BASE, &el, &created);
    if(err != 9) { fclose(f); return; }
    
    loaded = true;
      
    if(!created) { fclose(f); return; }
    
    for(int i=0; i<CELLS; i++) err = fscanf(f, "%d", &ct[i]);

    for(int i=0; i<CELLS; i++) for(int j=0; j<16; j++)
      err = fscanf(f, "%lf" ,&vx[i][j]);

    for(int i=0; i<CELLS; i++) 
    for(int j=0; j<7; j++) nei[i][j] = -1;

    while(true) {
      int a, b, c;      
      err = fscanf(f, "%d%d%d", &a, &b, &c);
      if(a < 0) break;
      else nei[a][c] = b;
      }
    
    for(int i=0; i<CELLS; i++) {
      double dx, dy, dr;
      int g;
      err = fscanf(f, "%lf%lf%lf%d\n", &dx, &dy, &dr, &g);
      center[i] = vec(dx, dy);
      rot[i] = dr;
      glued[i] = g;
      }
    
    fclose(f);
    }

  void saveData() {
    // global parameters
    FILE *f = fopen("papermodeldata2.txt", "wt");
    if(!f) {
      addMessage("Could not save the paper model data");
      return;
      }
    fprintf(f, "%d %d %d %d %d %d %d %lf %d\n\n", CELLS, SX, SY, PX, PY, SCALE, BASE, el, created);

    // net parameters: cell types    
    for(int i=0; i<CELLS; i++)
      fprintf(f, "%d ", ct[i]);
    fprintf(f, "\n");
    
    // net parameters: hcenters    
    for(int i=0; i<CELLS; i++) {
      for(int k=0; k<16; k++)
        fprintf(f, "%9.6lf ", vx[i][k]);
      fprintf(f, "\n");
      }
    fprintf(f, "\n\n");
    
    // create netgen
    for(int i=0; i<CELLS; i++) for(int j=0; j<CELLS; j++) {
      for(int k=0; k<ct[i]; k++) if(nei[i][k] == j)
        fprintf(f, "%d %d %d  ", i, j, k);
      }
    fprintf(f, "-1 -1 -1\n\n");
    
    // graphics
    for(int i=0; i<CELLS; i++) 
      fprintf(f, "%12.7lf %12.7lf %10.7lf %d\n",
        center[i].x, center[i].y, rot[i], glued[i]
        );
      
    fclose(f);
    }  

  // Simple graphical functions
  //============================
  
  void blackline(vec v1, vec v2, color_t col = 0x000000FF) {
#if CAP_SDLGFX==1
    aalineColor(s, int(v1.x), int(v1.y), int(v2.x), int(v2.y), col);
#endif
    }
  
  void drawtriangle(vec v1, vec v2, vec v3, color_t col) {
#if CAP_SDLGFX==1
    polyx[0] = int(v1.x);
    polyx[1] = int(v2.x);
    polyx[2] = int(v3.x);
    polyy[0] = int(v1.y);
    polyy[1] = int(v2.y);
    polyy[2] = int(v3.y);
    filledPolygonColorI(s, polyx, polyy, 3, col);
#endif
    }
    
  void blackcircle(vec v, int r, color_t col = 0x000000FF) {
#if CAP_SDLGFX
    aacircleColor(s, int(v.x), int(v.y), r, col);
#endif
    }
  
  void blacktext(vec v, char c) {
    char str[2]; str[0] = c; str[1] = 0;
    int tsize = int(el * 12/27);
    displaystr(int(v.x), int(v.y), 0, tsize, str, 0, 8);
    }
  
  hyperpoint hvec(int i, int e) {
    return hpxy(vx[i][2*e], vx[i][2*e+1]);
    }
  
  bool wellspread(double d1, double d2, double d3, int &co) {
    int id1 = int(d1);
    int id2 = int(d2);
    int id3 = int(d3);
    co = min(min(id1,id2),id3);
    return (id1 <= co+1 && id2 <= co+1 && id3 <= co+1);
    }
  
  SDL_Surface *net, *hqsurface;
  
  color_t& hqpixel(hyperpoint h) {
    int hx, hy, hs;
    getcoord0(h, hx, hy, hs);
    return qpixel(hqsurface, hx, hy);
    }
  
  void copyhypertriangle(
    vec g1, vec g2, vec g3,
    hyperpoint h1, hyperpoint h2, hyperpoint h3) {
    int ix, iy;

    if(wellspread(g1.x,g2.x,g3.x,ix) && wellspread(g1.y,g2.y,g3.y,iy))
      qpixel(net,ix,iy) = hqpixel(h1);
    else {
    
      vec g4 = (g2+g3)/2;
      vec g5 = (g3+g1)/2;
      vec g6 = (g1+g2)/2;
      
      hyperpoint h4 = mid(h2,h3);
      hyperpoint h5 = mid(h3,h1);
      hyperpoint h6 = mid(h1,h2);
      
      copyhypertriangle(g1,g5,g6, h1,h5,h6);
      copyhypertriangle(g5,g3,g4, h5,h3,h4);
      copyhypertriangle(g6,g4,g2, h6,h4,h2);
      copyhypertriangle(g4,g6,g5, h4,h6,h5);
      }
    }
  
  void setRaylen() {
    for(int i=0; i<CELLS; i++) {
      raylen[i] = el / sin(M_PI / ct[i]);
      edgist[i] = raylen[i] * cos(M_PI / ct[i]);
      }
    }
  
  // draw the model
  void createPapermodel() {

    #if !CAP_SDLGFX
      addMessage(XLAT("High quality shots not available on this platform"));
      return;
    #endif
    
    loadData();

    SDL_Surface *sav = s; 

    s = hqsurface = SDL_CreateRGBSurface(SDL_SWSURFACE,BASE,BASE,32,0,0,0,0);
    
    videopar vid2 = vid;
    vid.xres = vid.yres = 2000; vid.scale = 0.99; vid.usingGL = false;
    int sch = cheater; cheater = 0;
    calcparam();
    
    mode = 2;

    darken = 0;
    SDL_FillRect(s, NULL, 0);
    drawfullmap();

    mode = 0;
    
/*    for(int i=0; i<CELLS; i++) {
      int t = ct[i];  
      for(int e=0; e<t; e++) 
        drawline(hvec(i,e), hvec(i,(e+1)%t), 0x80808080);
      for(int e=0; e<7; e++) 
        drawline(hvec(i,e), hvec(i,7), 0x80808080);
      } */
    
    s = net = SDL_CreateRGBSurface(SDL_SWSURFACE,SX*SCALE,SY*SCALE,32,0,0,0,0);
    SDL_FillRect(net, NULL, 0xFFFFFF);
    
    int pateks = 0;
    
    int zeroi = nei[0][0];
    int zeroe = 0;
    for(int e=0; e<6; e++) if(nei[zeroi][e] == 0) zeroe = e;
    
    el *= SCALE;
    setRaylen();
    
    for(int faza=0; faza<2; faza++) for(int i=0; i<CELLS; i++) {
      
      int t = ct[i];
      
      printf("faza %d cell %d\n", faza, i);
  
      for(int e=0; e<t; e++) {
        vec v1 = center[i] * SCALE + raylen[i] * ang(rot[i] + 2*M_PI*e/t);
        vec v2 = center[i] * SCALE + raylen[i] * ang(rot[i] + 2*M_PI*(e+1)/t);
        vec v3 = (v1+v2)/2;
  
        if(faza == 1) blackline(v1, v2);
        
        int ofs = t == 7 ? 0 : 5;
        
        // 0,2,0 ~ 2,0,0
        
        if(0) if((i==0 && e == 0) || (i == zeroi && e == zeroe)) {
          for(int ofs=0; ofs<t; ofs++) {
            printf("OFS %d: %s", ofs, display(hvec(i, (e+ofs)%t)));
            printf(" %s\n", display(hvec(i, (e+1+ofs)%t)));
            }
          }

        if(faza == 0) copyhypertriangle(
          center[i] * SCALE, v1, v2,
          hvec(i,7), hvec(i, (e+ofs)%t), hvec(i, (e+1+ofs)%t)
          );
        
        if(faza == 1)
        if(nei[i][e] != -1 && nei[i][e] != glued[i] && glued[nei[i][e]] != i) {
          vec vd = v2-v1;
          swap(vd.x, vd.y); vd.x = -vd.x;
          double factor = -sqrt(3)/6;
          vd.x *= factor;
          vd.y *= factor;
          vec v4 = v3 + vd;
          vec v5 = v3 + vd/2;
          
          if(!patek[i][e]) {
            int i2 = nei[i][e];
            for(int e2=0; e2<ct[nei[i][e]]; e2++) if(nei[i2][e2] == i) 
              patek[i][e] = patek[i2][e2] = 
                "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"
                "!@#$%^&*+=~:;<>?/|\"., [{(\\]})" [(pateks++) % 85];
            }
          
          color_t col = 0xFFFFFFFF;
          int p = patek[i][e];
          col -= 0x8000 * (p&1); p /= 2;
          col -= 0x800000 * (p&1); p /= 2;
          col -= 0x80000000 * (p&1); p /= 2;
          col -= 0x4000 * (p&1); p /= 2;
          col -= 0x400000 * (p&1); p /= 2;
          col -= 0x40000000 * (p&1); p /= 2;
          col -= 0x2000 * (p&1); p /= 2;
          col -= 0x200000 * (p&1); p /= 2;
          col -= 0x20000000 * (p&1); p /= 2;
          
          drawtriangle(v1,v2,v4, col);
            
          blacktext(v5, patek[i][e]);

          blackline(v1, v4);
          blackline(v2, v4);          
          }
        }
      }
    
    printf("pateks = %d\n", pateks);
        
    IMAGESAVE(net, "papermodel-all" IMAGEEXT);
    IMAGESAVE(hqsurface, "papermodel-source" IMAGEEXT);

    int qx = SX*SCALE/PX;
    int qy = SY*SCALE/PY;
    SDL_Surface *quarter = SDL_CreateRGBSurface(SDL_SWSURFACE,qx,qy,32,0,0,0,0);
    for(int iy=0; iy<PY; iy++)
    for(int ix=0; ix<PX; ix++) {
      for(int y=0; y<qy; y++) for(int x=0; x<qx; x++)
        qpixel(quarter,x,y) = qpixel(net, x+qx*ix, y+qy*iy);
      char buf[64];
      sprintf(buf, "papermodel-page%d%d" IMAGEEXT, iy, ix);
      IMAGESAVE(quarter, buf);
      }
    
    SDL_FreeSurface(net);
    SDL_FreeSurface(hqsurface);
    SDL_FreeSurface(quarter);
    
    s = sav; vid = vid2; cheater = sch;
    }
  
  vec mousepos, rel;

  int bei = 0, bee = 0, whichcell = 0;
  double cedist;
  bool dragging = false;
  
  int glueroot(int i) {
    if(glued[i] == -1) return i;
    return glueroot(glued[i]);
    }
  
  void clicked(int x, int y, int b) {
  
    mousepos = vec(x, y);
    
    if(b == 1)
      rel = center[glueroot(whichcell)] - mousepos,
      dragging = true;
    
    if(b == 17)
      dragging = false;
    
    if(b == 32 && dragging)
      center[glueroot(whichcell)] = rel + mousepos;
    
    }
  
  void applyGlue(int i) {
    int j = glued[i];
    int it = ct[i];
    int jt = ct[j];
    int ie = 0, je = 0;
    for(int e=0; e<it; e++) if(nei[i][e] == j) ie = e;
    for(int e=0; e<jt; e++) if(nei[j][e] == i) je = e;
    
    rot[i] = rot[j] + 2*M_PI*(je+.5)/jt - 2*M_PI*(ie+.5)/it + M_PI;
    center[i] = 
      center[j] +
        (edgist[i]+edgist[j]) * ang(rot[j] + 2*M_PI*(je+.5)/jt);
    }

  void displaynets() {
    SDL_LockSurface(s);

    setRaylen();
    
    for(int uy=SY-1; uy>=0; uy--)
    for(int ux=SX-1; ux>=0; ux--) {
      qpixel(s, ux, uy) = 0;
      }
    
    for(int y=1; y<PY; y++)
      blackline(vec(0,SY*y/PY), vec(SX,SY*y/PY), 0x404080FF);

    for(int x=1; x<PX; x++)
      blackline(vec(SX*x/PX,0), vec(SX*x/PX,SY), 0x404080FF);
    
    for(int i=0; i<CELLS; i++) {
      
      if(norm(center[i]-mousepos) < norm(center[whichcell]-mousepos))
        whichcell = i;
  
      int t = ct[i];
  
      if(i == whichcell)
        blackcircle(center[i], 10, 0x40FF40FF);
  
      if(i == bei || i == nei[bei][bee])
        blackcircle(center[i], 5, 0x40FF40FF);
      
      if(glued[i] == -1)
        blackcircle(center[i], 7, 0xFF4040FF);
      
      if(glued[i] != -1) 
        applyGlue(i);
  
      for(int e=0; e<t; e++) {
        vec v1 = center[i] + raylen[i] * ang(rot[i] + 2*M_PI*e/t);
        vec v2 = center[i] + raylen[i] * ang(rot[i] + 2*M_PI*(e+1)/t);
        vec v3 = (v1+v2)/2;
  
        if(nei[i][e] >= 0 && !dragging) {
          if(norm(v3-mousepos) < cedist) bei = i, bee = e;
          if(i == bei && e == bee) cedist = norm(v3-mousepos);
          }

        color_t col = 
          i == bei && e == bee ? 0x40FF40FF:
          i == nei[bei][bee] && nei[i][e] == bei ? 0x40FF40FF :
          nei[i][e] == glued[i] ? 0x303030FF :
          glued[nei[i][e]] == i ? 0x303030FF :
          nei[i][e] >= 0 ? 0xC0C0C0FF : 
          0x808080FF;
  
        blackline(v1, v2, col);
        
        if(nei[i][e] != -1 && nei[i][e] != glued[i] && glued[nei[i][e]] != i) {
          vec vd = v2-v1;
          swap(vd.x, vd.y); vd.x = -vd.x;
          double factor = -sqrt(3)/6;
          vd.x *= factor; vd.y *= factor;
          vec v4 = v3 + vd;
          
          blackline(v1, v4, 0xFFC0C0C0);
          blackline(v2, v4, 0xFFC0C0C0);
          }
        }
      }
    
    SDL_UnlockSurface(s);
    SDL_UpdateRect(s, 0, 0, 0, 0);  
    }

  double rs, rz;
  
  void addglue() {
    int i = bei;
    int j = nei[bei][bee];
    if(glued[i] == j)
      glued[i] = -1;
    else if(glued[j] == i)
      glued[j] = -1;
    else if(glueroot(i) == glueroot(j))
      ;
    else if(glued[j] == -1)
      glued[j] = i;
    }
  
  int nti;
  
  void smooth() {
    int ti = SDL_GetTicks();
    rot[whichcell] += rs * (nti - ti) / 1000.0;
    
    el += rz * (nti - ti) / 1000.0;
    nti = ti;
    }
  
  void netgen_loop() {
    nti = SDL_GetTicks();
    while(true) {
      smooth();
      displaynets();
      SDL_Event event;
      
      while(SDL_PollEvent(&event)) switch (event.type) {
        case SDL_QUIT:
          exit(1);
          return;
    
        case SDL_MOUSEBUTTONDOWN: {
          clicked(event.button.x, event.button.y, event.button.button);
          break;
          }
        
        case SDL_MOUSEBUTTONUP: {
          clicked(event.button.x, event.button.y, 16+event.button.button);
          break;
          }
        
        case SDL_MOUSEMOTION: {
          clicked(event.motion.x, event.motion.y, 32);
          break;
          }
        
        case SDL_KEYDOWN: {
          int key = event.key.keysym.sym;
          int uni = event.key.keysym.unicode;
          
          if(uni == 'q' || key == SDLK_ESCAPE || key == SDLK_F10) 
            return;
          
          if(key == SDLK_PAGEUP) rs = 3;
          if(key == SDLK_PAGEDOWN) rs = -3;    
          if(uni == 'z') rz = 1;
          if(uni == 'x') rz = -1;          
          if(uni == 'g') addglue();
          
          break;
          }
    
        case SDL_KEYUP: {
          rs = 0;
          rz = 0;          
          break;
          }
        
        }
      }
    }
  
  void designNet() {
    s = SDL_SetVideoMode(SX, SY, 32, 0);
    netgen_loop();
    saveData();
    setvideomode();
    }

  void show() {
    cmode = sm::SIDE;
    gamescreen(0);
    if(true) {
      for(int i=0; i<CELLS; i++) {
        int t = ct[i];
        int ofs = t == 7 ? 0 : 5;
        for(int e=0; e<t; e++) {
          color_t col = 
            nei[i][e] == glued[i] && glued[i] >= 0 ? 0x303030 :
            nei[i][e] >= 0 && glued[nei[i][e]] == i ? 0x303030 :
            nei[i][e] >= 0 ? 0x808080 : 
            0xC0C0C0;
    
          prettyline(hvec(i, (e+ofs)%t), hvec(i, (e+1+ofs)%t), (col << 8) + 0xFF, 3);
          }
        }
      }
    if(mode != 2) {
      dialog::init("paper model creator");

      dialog::addItem(XLAT("synchronize net and map"), 's');
      dialog::addItem(XLAT("display the scope"), 't');  
      dialog::addItem(XLAT("create the model"), 'c');
      dialog::addItem(XLAT("design the net"), 'd');
      dialog::addBreak(50);
      dialog::addBack();
      
      dialog::display();
      }
    
    keyhandler = [] (int sym, int uni) {
      dialog::handleNavigation(sym, uni);
  
      if(!loaded) { 
        loadData(); 
        if(!loaded) { 
          addMessage(XLAT("Failed to load the file 'papermodeldata.txt'"));
          popScreen();
          return;
          }
        if(!created) {
          View = Id;
          if(centerover.at) viewctr.at = centerover.at->master;
          else viewctr.at = cwt.at->master;
          playermoved = false;
          dataFromHR();
          designNet();
          created = 1;
          return;
          }
        }
  
      if(mode == 2 && uni != 0) {
        mode = 0;
        return;
        }
      if(uni == 's') {
        View = Id;
        if(centerover.at) viewctr.at = centerover.at->master;
        else viewctr.at = cwt.at->master;
        playermoved = false;
        }
      else if(uni == 'c') {
        createPapermodel();
        addMessage(XLAT("The paper model created as papermodel-*.bmp"));
        }
      else if(uni == 'd') designNet();
      else if(uni == 't') mode = 2;
      else if(doexiton(sym, uni))
        popScreen();
      };
    }

  void run() { 
    if(euclid) 
      addMessage("Useless in Euclidean geometry.");
    else if(sphere)
      addMessage("Not implemented for spherical geometry. Please tell me if you really want this.");
    else
      pushScreen(show);
    }
  }}
#endif
