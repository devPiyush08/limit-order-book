#include "Engine.h"
#include <iostream>
#include <string>
#include <cstdio>
#include <cstdlib>

int main(int argc, char* argv[]) {
    // Force flush on Windows
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);

    Engine::Config cfg;
    cfg.symbol      = "AAPL";
    cfg.midPrice    = 15000;
    cfg.targetOps   = 5000;
    cfg.displaySecs = 3;
    cfg.runSecs     = 30;
    cfg.verbose     = false;

    for (int i = 1; i < argc; i++) {
        std::string arg(argv[i]);
        if (arg == "--symbol"  && i+1 < argc) { cfg.symbol      = argv[++i]; }
        else if (arg == "--ops"     && i+1 < argc) { cfg.targetOps  = (uint32_t)atoi(argv[++i]); }
        else if (arg == "--mid"     && i+1 < argc) { cfg.midPrice   = (Price)atoi(argv[++i]); }
        else if (arg == "--run"     && i+1 < argc) { cfg.runSecs    = atoi(argv[++i]); }
        else if (arg == "--display" && i+1 < argc) { cfg.displaySecs= atoi(argv[++i]); }
        else if (arg == "--verbose")               { cfg.verbose    = true; }
        else if (arg == "--help") {
            printf("\n=== HFT Order Book ===\n");
            printf("  --symbol  <STR>  e.g. AAPL\n");
            printf("  --ops     <INT>  ops/sec (default 5000)\n");
            printf("  --mid     <INT>  price in ticks (15000 = $150.00)\n");
            printf("  --run     <INT>  seconds to run (default 30)\n");
            printf("  --verbose        print every trade\n");
            return 0;
        }
    }

    printf("\n");
    printf("  ================================\n");
    printf("   HFT LIMIT ORDER BOOK  v1.0\n");
    printf("  ================================\n");
    printf("  Symbol  : %s\n",  cfg.symbol.c_str());
    printf("  Ops/sec : %u\n",  cfg.targetOps);
    printf("  Run     : %d seconds\n", cfg.runSecs);
    printf("  ================================\n\n");
    fflush(stdout);

    Engine engine(cfg);
    engine.run();

    return 0;
}