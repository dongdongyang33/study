#include "cachelab.h"
#include "stdlib.h"
#include "stdbool.h"
#include "stdio.h"
#include "string.h"

#define ULL unsigned long long
#define	MAX	4096

void getParameter(int argv, char** argvc);
void printHelp();
void mallocAndInitialize();
void freeCache();
void findAndAddIntoCache(ULL currentopt, char opt, ULL addr);

struct Cachecell
{
    ULL tag;
    ULL last_usetime;
};
struct Cachecell** cache;
int* cacheset;
bool verbose;
bool debug;
int help;
int hit_num, miss_num, evic_num;
int associativity, set_num, block_num;
char file[MAX];

void getParameter(int argc, char* argv[])
{
    help = 0;
    verbose = false;
    debug = false;
    associativity = -1;
    set_num = -1;
    block_num = -1;
    int cnt = 0;
    for(int i = 1; i < argc; i++)
    {
        if(argv[i][0] != '-')
        {
            help = 1;
            return;
        }
        if(argv[i][1] == 'E')
        {
            associativity = atoi(argv[i+1]);
            i++;
            cnt++;
            continue;
        }
        if(argv[i][1] == 's')
        {
            set_num = atoi(argv[i+1]);
            i++;
            cnt++;
            continue;
        }
        if(argv[i][1] == 'b')
        {
            block_num = atoi(argv[i+1]);
            i++;
            cnt++;
            continue;
        }
        if(argv[i][1] == 't')
        {
            strcpy(file, argv[i+1]);
            i++;
            cnt++;
            continue;
        }
        if(argv[i][1] == 'v')
        {
            verbose = true;
            continue;
        }
        if(argv[i][1] == 'h')
        {
            help = 1;
            return;
        }
        if(argv[i][1] == 'D')
        {
            debug = true;
            continue;
        }
    }
    if(cnt != 4) help = -1;
}

void printHelp()
{
    if(help == -1) printf("./csim-ref: Missing required command line argument\n");

    printf("Usage: ./csim-ref [-hv] -s <num> -E <num> -b <num> -t <file>\n");
    printf("Options:\n");
    printf("  -h         Print this help message.\n");
    printf("  -v         Optional verbose flag.\n");
    printf("  -s <num>   Number of set index bits.\n");
    printf("  -E <num>   Number of lines per set.\n");
    printf("  -b <num>   Number of block offset bits.\n");
    printf("  -t <file>  Trace file.\n\n");
    printf("Examples:\n");
    printf("  linux>  ./csim-ref -s 4 -E 1 -b 4 -t traces/yi.trace\n");
    printf("  linux>  ./csim-ref -v -s 8 -E 2 -b 4 -t traces/yi.trace\n");
}

void mallocAndInitialize()
{
    int s = 1 << set_num;
    cacheset = (int*)malloc(sizeof(int)*s);
    cache = (struct Cachecell**)malloc(sizeof(struct Cachecell*)*s);
    for(int i = 0; i < s; i++)
    {
        cache[i] = (struct Cachecell*)malloc(sizeof(struct Cachecell)*associativity);
        cacheset[i] = 0;
    }

    hit_num = 0;
    miss_num = 0;
    evic_num = 0;
}

void freeCache()
{
    free(cacheset);
    free(cache);
}

int getSet(ULL addr)
{
    ULL addr_ = addr;
    addr_ = (addr_ >> block_num);
    ULL get_ = 0xFFFFFFFFFFFFFFFF;
    get_ = ~(get_ << set_num);
    return (int)(addr_ & get_);
}

void findAndAddIntoCache(ULL currentopt, char opt, ULL addr)
{
    int optset = getSet(addr);
    ULL opttag = (addr >> (block_num + set_num));
    int bound = cacheset[optset];
    bool result = false;
    ULL last = currentopt;
    int evic = -1;
    for(int i = 0; i < bound; i++)
    {
        if(cache[optset][i].tag == opttag)
        {
            if(verbose) printf("hit ");
            hit_num++;
            cache[optset][i].last_usetime = currentopt;
            result = true;
        }
        else
        {
            if(cache[optset][i].last_usetime < last || evic == -1)
            {
                evic = i;
                last = cache[optset][i].last_usetime;
            }
        }
    }
    if(!result)
    {
        if(verbose) printf("miss ");
        miss_num++;
        if(bound == associativity)
        {
            if(verbose) printf("eviction ");
            evic_num++;
            cache[optset][evic].tag = opttag;
            cache[optset][evic].last_usetime = currentopt;
        }
        else
        {
            if(bound < associativity)
            {
                cache[optset][bound].tag = opttag;
                cache[optset][bound].last_usetime = currentopt;
                cacheset[optset]++;
            }
        }
    }
    if(opt == 'M')
    {
        if(verbose) printf("hit ");
        hit_num++;
    }
}

void showCacheInfo()
{
    int s = 1 << set_num;
    for(int i = 0; i < s; i++)
    {
        for(int j = 0; j < associativity; j++)
            printf("cache[%d][%d] = %llx(lastuse:%llx)\n",
                   i, j, cache[i][j].tag, cache[i][j].last_usetime);
    }
}

int main(int argc, char** argv)
{
    getParameter(argc, argv);
    if(help != 0)
    {
        printHelp();
        exit(0);
    }
    else
    {
        mallocAndInitialize();
        FILE *pFile;
        pFile = fopen(file, "r");

        char opt;
        ULL address;
        ULL currentopt = 0;
        int size;
        while(fscanf(pFile, " %c %llx,%d", &opt, &address, &size) > 0)
        {
            if(opt == 'I') continue;
            if(verbose) printf("%c %llx,%d ", opt, address, size);
            findAndAddIntoCache(currentopt, opt, address);
            if(verbose) printf("\n");
            if(debug) showCacheInfo();
            currentopt++;
        }
        fclose(pFile);
        freeCache();
    }
    printSummary(hit_num, miss_num, evic_num);

    return 0;
}
