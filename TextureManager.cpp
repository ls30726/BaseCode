#include "TextureManager.h"

TextureManager * TextureManager::s_pInstance = 0;
TextureManager::TextureManager() {};

bool TextureManager::load(const char* fileName, std::string textureID, SDL_Renderer* g_pRenderer) {
	
	SDL_Surface *image = IMG_Load(fileName);
	if (image == 0) return false;
	m_textureMap[textureID] = SDL_CreateTextureFromSurface(g_pRenderer, image);
	SDL_FreeSurface(image);
	
	return true;

};

void TextureManager::draw(std::string textureID, float x, float y, int width, int height, SDL_Renderer* g_pRenderer, SDL_RendererFlip flip) {
	SDL_RenderClear(g_pRenderer);

	SDL_Rect source, destination;

	source.x = 0;
	source.y = 0;
	source.w = width;
	source.h = height;

	destination.x = x;
	destination.y = y;
	destination.w = width;
	destination.h = height;
	SDL_RenderCopyEx(g_pRenderer, m_textureMap[textureID], &source, &destination, 0, 0, flip);
};

void TextureManager::drawFrame(std::string textureID, float x, float y, int width, int height, int currentRow, int currentFrame, SDL_Renderer* g_pRenderer, int flip) {

	SDL_Rect source, destination;
	SDL_RendererFlip rendererFlip;

	// Im�gen propiedades internas
	source.x = currentFrame * width;
	source.y = currentRow;
	source.w = width;
	source.h = height;

	// Imagen respecto a la ventana.
	destination.x = x;
	destination.y = y;
	destination.w = width;
	destination.h = height;

	if (flip == 0) rendererFlip = SDL_FLIP_NONE;
	if (flip == 1) rendererFlip = SDL_FLIP_HORIZONTAL;
	if (flip == 2) rendererFlip = SDL_FLIP_VERTICAL;

	SDL_RenderCopyEx(g_pRenderer, m_textureMap[textureID], &source, &destination, 0, 0, rendererFlip);
};

void TextureManager::drawTile(std::string textureID, int margin, int spacing, int x, int y, int width, int height, int currentRow,
	int currentFrame, SDL_Renderer *pRenderer)
{


	SDL_Rect source, destination;

	source.x = margin + (spacing + width) * currentFrame;
	source.y = margin + (spacing + height) * currentRow;
	source.w = width;
	source.h = height;

	// Imagen respecto a la ventana.
	destination.x = x;
	destination.y = y;
	destination.w = width;
	destination.h = height;

	SDL_RenderCopyEx(pRenderer, m_textureMap[textureID], &source, &destination, 0, 0, SDL_FLIP_NONE);
}

void TextureManager::clearFromTextureMap(std::string id)
{
	m_textureMap.erase(id);
}


