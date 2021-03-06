#include "LevelParser.h"
#include "TextureManager.h"
#include "GameObjectFactory.h"
#include "Game.h"
#include "base64.h"
#include "zlib.h"
#include "TileLayer.h"
#include "ObjectLayer.h"
#include "Camera.h"

Level* LevelParser::parseLevel(const char *levelFile)
{
	TiXmlDocument levelDocument;
	levelDocument.LoadFile(levelFile);

	Level* pLevel = new Level();
	
	TiXmlElement* pRoot = levelDocument.RootElement();
	pRoot->Attribute("tilewidth", &m_tileSizew);
	pRoot->Attribute("tileheight", &m_tileSizeh);
	pRoot->Attribute("width", &m_width);
	pRoot->Attribute("height", &m_height);

	for (TiXmlElement* e = pRoot->FirstChildElement(); e != NULL; e = e->NextSiblingElement())
	{
		if (e->Value() == std::string("tileset"))
		{
			parseTilesets(e, pLevel->getTilesets());
		}
		if (e->Value() == std::string("properties"))
		{
			TiXmlElement* pproperties = e;
			for (TiXmlElement* e = pproperties->FirstChildElement(); e != NULL; e = e->NextSiblingElement())
			{
				if (e->Value() == std::string("property"))
				{
					parseTextures(e);
				}
			}
		}
		if (e->Value() == std::string("objectgroup") || e->Value() == std::string("layer"))
		{
			if (e->FirstChildElement()->Value() == std::string("object"))
			{
				parseObjectLayer(e, pLevel->getLayers(), pLevel);
			}
			else if (e->FirstChildElement()->Value() == std::string("data") || (e->FirstChildElement()->NextSiblingElement() != 0
				&& e ->FirstChildElement()->NextSiblingElement()->Value() == std::string("data")))
			{
				parseTileLayer(e, pLevel->getLayers(), pLevel->getTilesets(), pLevel->getCollisionLayers());
			}
		}
	}

	return pLevel;
}

void LevelParser::parseTilesets(TiXmlElement * pTilesetRoot, std::vector<Tileset>* pTilesets)
{
	TextureManager::Instance()->load(pTilesetRoot->FirstChildElement()->Attribute("source"),
		pTilesetRoot->Attribute("name"), Game::Instance()->getRenderer());

	Tileset tileset;
	pTilesetRoot->FirstChildElement()->Attribute("width",
		&tileset.width);
	pTilesetRoot->FirstChildElement()->Attribute("height",
		&tileset.height);
	pTilesetRoot->Attribute("firstgid", &tileset.firstGridID);
	pTilesetRoot->Attribute("tilewidth", &tileset.tileWidth);
	pTilesetRoot->Attribute("tileheight", &tileset.tileHeight);
	pTilesetRoot->Attribute("spacing", &tileset.spacing);
	pTilesetRoot->Attribute("margin", &tileset.margin);
	tileset.name = pTilesetRoot->Attribute("name");
	tileset.numColumns = tileset.width / (tileset.tileWidth +
		tileset.spacing);
	pTilesets->push_back(tileset);
}


void LevelParser::parseTileLayer(TiXmlElement * pTileElement, std::vector<Layer*>* pLayers, const std::vector<Tileset>* pTilesets, std::vector<TileLayer*> *pCollisionLayers)
{
	TileLayer* pTileLayer = new TileLayer(m_tileSizew, m_tileSizeh, *pTilesets);
	bool collidable = false;
	std::vector<std::vector<int>> data;
	std::string decodedIDs;
	TiXmlElement* pDataNode;
	for (TiXmlElement* e = pTileElement->FirstChildElement(); e !=
		NULL; e = e->NextSiblingElement())
	{
		if (e->Value() == std::string("properties"))
		{
			for (TiXmlElement* property = e->FirstChildElement(); property != NULL; property = property->NextSiblingElement())
			{
				if (property->Value() == std::string("property"))
				{
					if (property->Attribute("name") == std::string("collidable"))
					{
						collidable = true;
					}
				}
			}
		}
		if (e->Value() == std::string("data"))
		{
			pDataNode = e;
		}
	}
	for (TiXmlNode* e = pDataNode->FirstChild(); e != NULL; e =
		e->NextSibling())
	{
		TiXmlText* text = e->ToText();
		std::string t = text->Value();
		decodedIDs = base64_decode(t);
	}

	uLongf numGids = m_width * m_height * sizeof(int);
	std::vector<unsigned> gids(numGids);
	uncompress((Bytef*)&gids[0], &numGids, (const
		Bytef*)decodedIDs.c_str(), decodedIDs.size());
	std::vector<int> layerRow(m_width);
	for (int j = 0; j < m_height; j++)
	{
		data.push_back(layerRow);
	}
	for (int rows = 0; rows < m_height; rows++)
	{
		for (int cols = 0; cols < m_width; cols++)
		{
			data[rows][cols] = gids[rows * m_width + cols];
		}
	}
	if (collidable)
	{
		pCollisionLayers->push_back(pTileLayer);
	}

	pTileLayer->setTileIDs(data);
	pLayers->push_back(pTileLayer);
}

void LevelParser::parseTextures(TiXmlElement * pTextureRoot)
{
	TextureManager::Instance()->load(pTextureRoot->Attribute("value"), pTextureRoot->Attribute("name"), Game::Instance()->getRenderer());
}

void LevelParser::parseObjectLayer(TiXmlElement * pObjectElement, std::vector<Layer*>* pLayers, Level* pLevel)
{
	ObjectLayer* pObjectLayer = new ObjectLayer();
	for (TiXmlElement* e = pObjectElement->FirstChildElement(); e !=
		NULL; e = e->NextSiblingElement())
	{
		if (e->Value() == std::string("object"))
		{
			int x, y, width, height, numSprites, callbackId, currentRow, flip;
			char* textureID;
			Vector2D position, velocity, maxVelocity, acceleration, friction;
			int px, py;
			int vx, vy;
			int mvx, mvy;
			int ax, ay;
			int fx, fy;
			int m_collisionMargin;

			e->Attribute("x", &x);
			e->Attribute("y", &y);
			e->Attribute("width", &width);
			e->Attribute("height", &height);
			GameObject* pGameObject = GameObjectFactory::Instance()->CreateGameObject(e->Attribute("type"));

			for (TiXmlElement* properties = e->FirstChildElement();
			properties != NULL; properties = properties->NextSiblingElement())
			{
				if (properties->Value() == std::string("properties"))
				{
					for (TiXmlElement* property = properties->FirstChildElement(); property != NULL; property = property->NextSiblingElement())
					{
						if (property->Value() == std::string("property"))
						{
							if (property->Attribute("name") == std::string("textureWidth"))
							{
								property->Attribute("value", &width);
							}
							else if (property->Attribute("name") == std::string("textureHeight"))
							{
								property->Attribute("value", &height);
							}							
							else if (property->Attribute("name") == std::string("textureID"))
							{
								const size_t len = strlen(property->Attribute("value"));
								textureID = new char[len + 1];
								strncpy(textureID, property->Attribute("value"), len);
								textureID[len] = '\0';
							}							
							else if (property->Attribute("name") == std::string("numFrames"))
							{
								property->Attribute("value", &numSprites);
							}
							else if (property->Attribute("name") == std::string("callbackId"))
							{
								property->Attribute("value", &callbackId);
							}
							else if (property->Attribute("name") == std::string("px"))
							{
								property->Attribute("value", &px);
							}
							else if (property->Attribute("name") == std::string("py"))
							{
								property->Attribute("value", &py);
							}
							else if (property->Attribute("name") == std::string("vx"))
							{
								property->Attribute("value", &vx);
							}
							else if (property->Attribute("name") == std::string("vy"))
							{
								property->Attribute("value", &vy);
							}
							else if (property->Attribute("name") == std::string("mvx"))
							{
								property->Attribute("value", &mvx);
							}
							else if (property->Attribute("name") == std::string("mvy"))
							{
								property->Attribute("value", &mvy);
							}
							else if (property->Attribute("name") == std::string("ax"))
							{
								property->Attribute("value", &ax);
							}
							else if (property->Attribute("name") == std::string("ay"))
							{
								property->Attribute("value", &ay);
							}
							else if (property->Attribute("name") == std::string("fx"))
							{
								property->Attribute("value", &fx);
							}
							else if (property->Attribute("name") == std::string("fy"))
							{
								property->Attribute("value", &fy);
							}
							else if (property->Attribute("name") == std::string("currentRow"))
							{
								property->Attribute("value", &currentRow);
							}
							else if (property->Attribute("name") == std::string("flip"))
							{
								property->Attribute("value", &flip);
							}
							else if (property->Attribute("name") == std::string("m_collisionMargin"))
							{
								property->Attribute("value", &m_collisionMargin);
							}
						}
					}
				}
			}
			if (dynamic_cast<CollisionObject*>(pGameObject))
			{
				CollisionObject* pCollisionObj = dynamic_cast< CollisionObject* >(pGameObject);
				pCollisionObj->setCollisionLayers(pLevel->getCollisionLayers());
				pCollisionObj->setCollisionMargin(m_collisionMargin);
				if (dynamic_cast<Player*>(pGameObject)) {
					TheCamera::Instance()->setTarget(dynamic_cast<Player*>(pGameObject));
					TheCamera::Instance()->setPosition(dynamic_cast<Player*>(pGameObject)->getPosition());
				}
			}

			position = *(new Vector2D(px, py));
			velocity = *(new Vector2D(vx, vy));
			maxVelocity = *(new Vector2D(mvx, mvy));
			acceleration = *(new Vector2D(ax, ay));
			friction = *(new Vector2D(fx, fy));

			pGameObject->load(
				new LoaderParams(width, height, textureID, currentRow,numSprites, flip, callbackId, position, velocity, maxVelocity, acceleration, friction));
			pObjectLayer->getGameObjects()->push_back(pGameObject);
			if (textureID == "Player")
			{
				pLevel->setPlayer(dynamic_cast<Player*>(pGameObject));
			}			
			pObjectLayer->getGameObjects()->push_back(pGameObject);
		}
	}
	pLayers->push_back(pObjectLayer);
}