#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <math.h>
#include <stdbool.h>

#define CHILDREN (4)

#define TITLE ("EBOXYPIC")

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
};

struct node
{
    struct node* node[CHILDREN];
    struct quad quad;
    int depth;
};

static inline struct colo
Colo(uint32_t color)
{
    return (struct colo) {
        (color >> 0x10) & 0xFF,
        (color >> 0x08) & 0xFF,
        (color >> 0x00) & 0xFF,
    };
}

static inline struct colo
add(struct colo a, struct colo b)
{
    return (struct colo) {
        a.r + b.r,
        a.g + b.g,
        a.b + b.b,
    };
}

static inline double
mag(struct colo a)
{
    return sqrt(
        a.r * a.r +
        a.g * a.g +
        a.b * a.b);
}

static inline double
diff(struct colo a, struct colo b)
{
    return fabs(mag(a) - mag(b));
}

static inline struct colo
divide(struct colo a, int n)
{
    return (struct colo) {
        a.r / n,
        a.g / n,
        a.b / n,
    };
}

static inline struct quad
Quad(int x0, int y0, int x1, int y1, SDL_Surface* image)
{
    struct quad quad = { { { x0, y0 }, { x1, y1 } }, Colo(0x0) };
    uint32_t* pixels = image->pixels;
    struct colo colo = Colo(0x0);
    int count = 0;
    for(int y = y0; y < y1; y++)
    for(int x = x0; x < x1; x++)
    {
        uint32_t pixel = pixels[x + image->w * y];
        colo = add(colo, Colo(pixel));
        count += 1;
    }
    quad.colo = divide(colo, count);
    return quad;
}

static inline struct node*
Node(struct quad quad, SDL_Surface* image, int depth)
{
    struct node* node = calloc(1, sizeof(*node));
    node->quad = quad;
    node->depth = depth;
    if(node->depth < max_depth)
    {
        int next = depth + 1;
        int mx = (quad.rect.b.x + quad.rect.a.x) / 2;
        int my = (quad.rect.b.y + quad.rect.a.y) / 2;
        struct quad a = Quad(quad.rect.a.x, quad.rect.a.y, mx,            my,            image);
        struct quad b = Quad(mx,            quad.rect.a.y, quad.rect.b.x, my,            image);
        struct quad c = Quad(quad.rect.a.x, my,            mx,            quad.rect.b.y, image);
        struct quad d = Quad(mx,            my,            quad.rect.b.x, quad.rect.b.y, image);
        if(diff(a.colo, node->quad.colo) > max_diff
        || diff(b.colo, node->quad.colo) > max_diff
        || diff(c.colo, node->quad.colo) > max_diff
        || diff(d.colo, node->quad.colo) > max_diff)
        {
            node->node[0] = Node(a, image, next);
            node->node[1] = Node(b, image, next);
            node->node[2] = Node(c, image, next);
            node->node[3] = Node(d, image, next);
        }
    }
    return node;
}

static inline void
_Node(struct node* node)
{
    if(node)
    {
        for(int i = 0; i < CHILDREN; i++)
            _Node(node->node[i]);
        free(node);
    }
}

static inline bool
drawable(struct node* node)
{
    return node->node[0] == NULL
        || node->node[1] == NULL
        || node->node[2] == NULL
        || node->node[3] == NULL;
}

static inline void
draw(struct node* node, SDL_Renderer* renderer, bool outline)
{
    if(node)
    {
        for(int i = 0; i < CHILDREN; i++)
            draw(node->node[i], renderer, outline);
        if(drawable(node))
        {
            struct colo* colo = &node->quad.colo;
            struct rect* rect = &node->quad.rect;
            SDL_SetRenderDrawColor(renderer, colo->r, colo->g, colo->b, 0);
            SDL_Rect dst = {
                rect->a.x,
                rect->a.y,
                rect->b.x - rect->a.x,
                rect->b.y - rect->a.y,
            };
            SDL_RenderFillRect(renderer, &dst);
            if(outline)
            {
                SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
                SDL_RenderDrawRect(renderer, &dst);
            }
        }
    }
}

int main(void)
{
    const char* path = "./img.jpg";
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
        if(key[SDL_SCANCODE_E]) max_diff -= 0.1;
        if(key[SDL_SCANCODE_Q]) max_diff += 0.1;
        if(key[SDL_SCANCODE_W]) outline = false;
        struct node* node = Node(Quad(0, 0, image->w, image->h, image), image, 0);
        draw(node, renderer, outline);
        SDL_RenderPresent(renderer);
        _Node(node);
        SDL_PumpEvents();
        SDL_Delay(10);
    }
    SDL_DestroyRenderer(renderer);
    SDL_FreeSurface(surface);
    SDL_FreeSurface(image);
    SDL_DestroyWindow(window);
    SDL_Quit();
}
