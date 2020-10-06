#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <math.h>
#include <stdbool.h>

#define CHILDREN (4)

#define TITLE ("QUADCUTTER")

static double max_diff = 5.0;

static const int max_depth = 7;

struct colo
{
    int r;
    int g;
    int b;
};

struct point
{
    int x;
    int y;
};

struct rect
{
    struct point a;
    struct point b;
};

struct quad
{
    struct rect rect;
    struct colo colo;
    struct colo grey;
    int shade;
};

struct node
{
    struct node* node[CHILDREN];
    struct quad quad;
    int depth;
};

static inline struct colo
cl(uint32_t color)
{
    return (struct colo) { (color >> 0x10) & 0xFF, (color >> 0x08) & 0xFF, color & 0xFF };
}

static inline struct colo
cladd(struct colo a, struct colo b)
{
    return (struct colo) { a.r + b.r, a.g + b.g, a.b + b.b };
}

static inline double
clmag(struct colo a)
{
    return sqrt(a.r * a.r + a.g * a.g + a.b * a.b);
}

static inline double
cldiff(struct colo a, struct colo b)
{
    return fabs(clmag(a) - clmag(b));
}

static inline struct colo
cldiv(struct colo a, int n)
{
    return (struct colo) { a.r / n, a.g / n, a.b / n };
}

static inline struct quad
qd(int x0, int y0, int x1, int y1, SDL_Surface* image)
{
    struct quad quad = { 0 };
    quad.rect = (struct rect) { { x0, y0 }, { x1, y1 } };
    uint32_t* pixels = image->pixels;
    int count = 0;
    for(int y = y0; y < y1; y++)
    for(int x = x0; x < x1; x++)
    {
        uint32_t pixel = pixels[x + image->w * y];
        quad.colo = cladd(quad.colo, cl(pixel));
        count += 1;
    }
    quad.colo = cldiv(quad.colo, count);
    quad.shade = (quad.colo.r + quad.colo.g + quad.colo.b) / 3;
    quad.grey = (struct colo) { quad.shade, quad.shade, quad.shade };
    return quad;
}

static inline struct node*
nd(struct quad quad, SDL_Surface* image, int depth)
{
    struct node* node = calloc(1, sizeof(*node));
    node->quad = quad;
    node->depth = depth;
    if(node->depth < max_depth)
    {
        int next = depth + 1;
        int mx = (quad.rect.b.x + quad.rect.a.x) / 2;
        int my = (quad.rect.b.y + quad.rect.a.y) / 2;
        struct quad a = qd(quad.rect.a.x, quad.rect.a.y, mx,            my,            image);
        struct quad b = qd(mx,            quad.rect.a.y, quad.rect.b.x, my,            image);
        struct quad c = qd(quad.rect.a.x, my,            mx,            quad.rect.b.y, image);
        struct quad d = qd(mx,            my,            quad.rect.b.x, quad.rect.b.y, image);
        if(cldiff(a.colo, node->quad.colo) > max_diff
        || cldiff(b.colo, node->quad.colo) > max_diff
        || cldiff(c.colo, node->quad.colo) > max_diff
        || cldiff(d.colo, node->quad.colo) > max_diff)
        {
            node->node[0] = nd(a, image, next);
            node->node[1] = nd(b, image, next);
            node->node[2] = nd(c, image, next);
            node->node[3] = nd(d, image, next);
        }
    }
    return node;
}

static inline void
_nd_(struct node* node)
{
    if(node)
    {
        for(int i = 0; i < CHILDREN; i++)
            _nd_(node->node[i]);
        free(node);
    }
}

static inline bool
nddrawable(struct node* node)
{
    return node->node[0] == NULL || node->node[1] == NULL
        || node->node[2] == NULL || node->node[3] == NULL;
}

static inline void
nddraw(struct node* node, SDL_Renderer* renderer, bool outline, bool grey, bool fill)
{
    if(node)
    {
        for(int i = 0; i < CHILDREN; i++)
            nddraw(node->node[i], renderer, outline, grey, fill);
        if(nddrawable(node))
        {
            struct colo* colo = grey ? &node->quad.grey : &node->quad.colo;
            struct rect* rect = &node->quad.rect;
            if(fill)
                SDL_SetRenderDrawColor(renderer, colo->r, colo->g, colo->b, 0);
            else
                SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0);
            SDL_Rect dst = { rect->a.x, rect->a.y, rect->b.x - rect->a.x, rect->b.y - rect->a.y };
            SDL_RenderFillRect(renderer, &dst);
            if(outline)
            {
                SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
                SDL_RenderDrawRect(renderer, &dst);
            }
        }
    }
}

int main(int argc, char* argv[])
{
    if(argc == 2)
    {
        const char* path = argv[1];
        SDL_Init(SDL_INIT_VIDEO);
        SDL_PixelFormat* allocation = SDL_AllocFormat(SDL_PIXELFORMAT_RGB888);
        SDL_Surface* surface = IMG_Load(path);
        SDL_Surface* image = SDL_ConvertSurface(surface, allocation, 0);
        SDL_FreeFormat(allocation);
        SDL_Window* window;
        SDL_Renderer* renderer;
        SDL_CreateWindowAndRenderer(image->w, image->h, 0, &window, &renderer);
        const uint8_t* key = SDL_GetKeyboardState(NULL);
        bool done = false;
        while(!done)
        {
            done = key[SDL_SCANCODE_END] || key[SDL_SCANCODE_ESCAPE];
            bool outline = true;
            bool grey = false;
            bool fill = true ;
            if(key[SDL_SCANCODE_E]) max_diff -= 0.1;
            if(key[SDL_SCANCODE_Q]) max_diff += 0.1;
            if(key[SDL_SCANCODE_W]) outline = false;
            if(key[SDL_SCANCODE_R]) fill = false;
            if(key[SDL_SCANCODE_F]) grey = true;
            struct node* node = nd(qd(0, 0, image->w, image->h, image), image, 0);
            nddraw(node, renderer, outline, grey, fill);
            SDL_RenderPresent(renderer);
            _nd_(node);
            SDL_PumpEvents();
            SDL_Delay(10);
        }
        SDL_DestroyRenderer(renderer);
        SDL_FreeSurface(surface);
        SDL_FreeSurface(image);
        SDL_DestroyWindow(window);
        SDL_Quit();
    }
}
