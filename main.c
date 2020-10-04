#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <math.h>
#include <stdbool.h>

#define CHILDREN (4)

#define TITLE ("EBOXYPIC")

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
    uint32_t color;
};

struct node
{
    struct node* node[CHILDREN];
    struct quad quad;
    int depth;
};

struct colo
Colo(uint32_t color)
{
    return (struct colo) {
        (color >> 0x10) & 0xFF,
        (color >> 0x08) & 0xFF,
        (color >> 0x00) & 0xFF,
    };
}

uint32_t
unpack(struct colo colo)
{
    return colo.r << 0x10
         | colo.g << 0x08
         | colo.b << 0x00;
}

struct colo
add(struct colo a, struct colo b)
{
    return (struct colo) {
        a.r + b.r,
        a.g + b.g,
        a.b + b.b,
    };
}

double
mag(struct colo a)
{
    return sqrt(
        a.r * a.r +
        a.g * a.g +
        a.b * a.b);
}

double
diff(uint32_t a, uint32_t b)
{
    return fabs(mag(Colo(a)) - mag(Colo(b)));
}

struct colo
divide(struct colo a, int n)
{
    return (struct colo) {
        a.r / n,
        a.g / n,
        a.b / n,
    };
}

struct quad
Quad(int x0, int y0, int x1, int y1, SDL_Surface* image)
{
    struct quad quad = { { { x0, y0 }, { x1, y1 } }, 0x0 };
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
    colo = divide(colo, count);
    quad.color = unpack(colo);
    return quad;
}

struct node*
Node(struct quad quad, SDL_Surface* image, int depth)
{
    double max_diff = 5.0;
    int max_depth = 7;
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
        if(diff(a.color, node->quad.color) > max_diff
        || diff(b.color, node->quad.color) > max_diff
        || diff(c.color, node->quad.color) > max_diff
        || diff(d.color, node->quad.color) > max_diff)
        {
            node->node[0] = Node(a, image, next);
            node->node[1] = Node(b, image, next);
            node->node[2] = Node(c, image, next);
            node->node[3] = Node(d, image, next);
        }
    }
    return node;
}

void
_Node(struct node* node)
{
    if(node)
    {
        for(int i = 0; i < CHILDREN; i++)
            _Node(node->node[i]);
        free(node);
    }
}

uint32_t* lock(SDL_Texture* texture)
{
    int pitch;
    void* raw;
    SDL_LockTexture(texture, NULL, &raw, &pitch);
    return raw;
}

void xfer(uint32_t* pixels, struct rect* rect, uint32_t color, SDL_Surface* image)
{
    for(int x = rect->a.x; x < rect->b.x; x++)
    for(int y = rect->a.y; y < rect->b.y; y++)
        pixels[x + y * image->w] =
#if 1
            x == rect->b.x - 1 ||
            y == rect->b.y - 1 ? 0x0 : color;
#else
            color;
#endif
}

bool drawable(struct node* node)
{
    return node->node[0] == NULL
        || node->node[1] == NULL
        || node->node[2] == NULL
        || node->node[3] == NULL;
}

void
draw(struct node* node, SDL_Texture* texture, SDL_Surface* image)
{
    uint32_t* pixels = lock(texture);
    if(node)
    {
        for(int i = 0; i < CHILDREN; i++)
            draw(node->node[i], texture, image);
        if(drawable(node))
            xfer(pixels, &node->quad.rect, node->quad.color, image);
    }
    SDL_UnlockTexture(texture);
}

int main(void)
{
    const char* path = "./img.jpg";
    SDL_Init(SDL_INIT_VIDEO);
    SDL_PixelFormat* allocation = SDL_AllocFormat(SDL_PIXELFORMAT_RGB888);
    SDL_Surface* surface = IMG_Load(path);
    SDL_Surface* image = SDL_ConvertSurface(surface, allocation, 0);
    SDL_FreeFormat(allocation);
    SDL_Window* window = SDL_CreateWindow(
        TITLE,
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        image->w,
        image->h,
        SDL_WINDOW_SHOWN);
    SDL_Renderer* renderer = SDL_CreateRenderer(
        window,
        -1,
        SDL_RENDERER_ACCELERATED);
    SDL_Texture* texture = SDL_CreateTexture(
        renderer,
        SDL_PIXELFORMAT_RGB888,
        SDL_TEXTUREACCESS_STREAMING,
        image->w,
        image->h);
    struct node* node = Node(Quad(0, 0, image->w, image->h, image), image, 0);
    draw(node, texture, image);
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);
    _Node(node);
    const uint8_t* key = SDL_GetKeyboardState(NULL);
    while(!key[SDL_SCANCODE_END])
    {
        SDL_PumpEvents();
        SDL_Delay(15);
    }
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_FreeSurface(surface);
    SDL_FreeSurface(image);
    SDL_DestroyWindow(window);
    SDL_Quit();
}
