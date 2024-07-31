#define OLC_PGE_APPLICATION
#include "olcPixelGameEngine.h"
#include <vector>
using namespace std;


constexpr double PI = 3.1415926535;
constexpr double AngleC = PI / 4.0;
constexpr double AngleD = AngleC * 3.0;
constexpr double AngleB = -AngleC;
constexpr double AngleA = -AngleD;

struct Player {
	float x;
	float y;
	float angle; // In radians

	olc::vf2d getLookDir() {
		return olc::vf2d(cosf(x), sinf(y));
	}
};

class GameObject {
public:
	olc::Sprite* sprite;
	olc::vf2d pos;
	float scale = 1;
	bool bRemoved = false;

	GameObject(olc::Sprite* sprite, float x, float y) : pos(x,y) {
		this->sprite = sprite;
	}

	virtual void update(float elapsedTime) {}
};

class MovingGameObject : public GameObject {
public:
	olc::vf2d v;

	MovingGameObject(olc::Sprite* sprite, float x, float y, float vx, float vy) : GameObject(sprite,x,y), v(vx,vy) {
	}

	virtual void update(float elapsedTime) override {
		pos.x += v.x * elapsedTime;
		pos.y += v.y * elapsedTime;
	}
};

class Fireball : public MovingGameObject {
private:
	const vector<vector<int>>& gameMap;
	const olc::vi2d& gameSize;
public:
	Fireball(olc::Sprite* sprite, float x, float y, float vx, float vy, vector<vector<int>>& gameMap, olc::vi2d& gameSize) : MovingGameObject(sprite,x,y,vx,vy), gameMap(gameMap), gameSize(gameSize) {}

	void update(float elapsedTime) override {
		MovingGameObject::update(elapsedTime);
		if (pos.x < 0 || pos.x >= gameSize.x || pos.y < 0 || pos.y >= gameSize.y || gameMap[(int)pos.y][(int)pos.x] == 1) {
			bRemoved = true;
		}
	}
};


struct RaycastResult { bool bHit; float distance; }	cast_ray(const olc::vf2d& start, const olc::vf2d& dir, const std::vector<std::vector<int>>& gameMap, const olc::vi2d& mapSize, float maxDistance = 100.0f) {
	olc::vf2d unitHypotStep(sqrtf(1 + powf(dir.y / dir.x, 2.0f)), sqrtf(1 + powf(dir.x / dir.y, 2.0f)));
	olc::vi2d unitStep;
	olc::vf2d hypotLength(0, 0);
	olc::vi2d mapCheck = start;

	if (dir.x == 0) {
		unitHypotStep.x = maxDistance;
		hypotLength.x = maxDistance;
	}
	else if (dir.y == 0) {
		unitHypotStep.y = maxDistance;
		hypotLength.y = maxDistance;
	}

	if (dir.x > 0) {
		unitStep.x = 1;
		hypotLength.x += (float(mapCheck.x+1) - start.x) * unitHypotStep.x;
	}
	else {
		unitStep.x = -1;
		hypotLength.x += (start.x - float(mapCheck.x)) * unitHypotStep.x;
	}

	if (dir.y > 0) {
		unitStep.y = 1;
		hypotLength.y += (float(mapCheck.y+1) - start.y) * unitHypotStep.y;
	}
	else {
		unitStep.y = -1;
		hypotLength.y += (start.y - float(mapCheck.y)) * unitHypotStep.y;
	}

	float distance = 0.0f;
	bool bHit = false;
	while (!bHit && distance < maxDistance) {
		if (hypotLength.x < hypotLength.y) {
			mapCheck.x += unitStep.x;
			distance = hypotLength.x;
			hypotLength.x += unitHypotStep.x;
		}
		else {
			mapCheck.y += unitStep.y;
			distance = hypotLength.y;
			hypotLength.y += unitHypotStep.y;
		}

		if (mapCheck.x >= 0 && mapCheck.x < mapSize.x && mapCheck.y >= 0 && mapCheck.y < mapSize.y) {
			if (gameMap[mapCheck.y][mapCheck.x] == 1) {
				bHit = true;
			}
		}
	}

	return { bHit, distance };
}

class RaycastDebug : public olc::PixelGameEngine {
	vector<vector<int>> field;
	int blockSize = 30;
	int rows;
	int cols;
	olc::vf2d pPos;

	bool OnUserCreate() override {
		rows = ScreenHeight() / blockSize;
		cols = ScreenWidth() / blockSize;

		/*for (int x = 0; x < cols; x++) {
			vector<int> v;
			field.push_back(v);
			for (int y = 0; y < rows; y++) {
				v.push_back(0);
			}
		}*/

		field = vector<vector<int>>(rows, vector<int>(cols, 0));
		pPos.x = ScreenWidth() / 2;
		pPos.y = ScreenHeight() / 2;

		return true;
	}

	bool OnUserUpdate(float elapsedTime) override {
		Clear(olc::BLACK);

		const float speed = 100;
		if (GetKey(olc::A).bHeld)
			pPos.x -= speed * elapsedTime;
		if (GetKey(olc::D).bHeld)
			pPos.x += speed * elapsedTime;
		if (GetKey(olc::W).bHeld)
			pPos.y -= speed * elapsedTime;
		if (GetKey(olc::S).bHeld)
			pPos.y += speed * elapsedTime;


		for (int x = 0; x < cols; x++) {
			for (int y = 0; y < rows; y++) {
				if (field[y][x] == 1) {
					FillRect({ x * blockSize,y * blockSize }, { blockSize,blockSize }, olc::BLUE);
				}
				DrawRect({ x*blockSize,y*blockSize }, { blockSize,blockSize }, olc::WHITE);
			}
		}

		if (GetMouse(olc::Mouse::RIGHT).bHeld) {
			field[GetMouseY()/blockSize][GetMouseX()/blockSize] = 1;
		}

		if (GetMouse(olc::Mouse::LEFT).bHeld) {
			olc::vf2d mousePos = GetMousePos();
			olc::vf2d dir = mousePos - pPos;
			dir = dir.norm();
			

			olc::vf2d nPPos = pPos / blockSize;
			//olc::vf2d nMousePos = mousePos / blockSize;

			std::cout << olc::vi2d(cols, rows).str() << ' ' << nPPos.str() << '\n';

			auto result = cast_ray(nPPos, dir, field, olc::vi2d(cols,rows));
			
			olc::vf2d end = dir * result.distance*blockSize + pPos;
			DrawLine(pPos, end, olc::YELLOW);
			DrawCircle(end, 5, olc::YELLOW);
			
		}

		FillCircle(pPos, 10, olc::GREEN);
		FillCircle(GetMousePos(), 10, olc::RED);

		return true;
	}
};


// Override base class with your custom functionality
class Game : public olc::PixelGameEngine
{
private:
	vector<vector<int>> gameMap = {
		{1,1,1,1,1,1,1},
		{1,0,0,0,0,0,1},
		{1,0,0,0,0,0,1},
		{1,0,0,0,0,0,1},
		{1,0,0,0,0,0,1},
		{1,0,1,1,1,0,1},
		{1,0,0,1,0,0,1},
		{1,0,1,1,1,0,1},
		{1,0,0,0,0,0,1},
		{1,1,1,1,1,1,1},
	};
	float* depthBuffer;

	int H = gameMap.size();
	int W = gameMap[0].size();
	olc::vi2d gameSize = olc::vi2d(W, H);

	float FOV = 3.14 / 4;
	float HFOV = FOV / 2;
	float rayCount = 100;
	float deltaFOV = FOV / rayCount;
	float MAX_DISTANCE = 16;

	olc::Sprite* wallTexture;
	olc::Sprite* lampTexture;
	olc::Sprite* fireballTexture;

	std::vector<GameObject*> gameObjects;

	void Draw(int x, int y, const olc::Pixel& color, float distance) {
		if (x < 0 || x >= ScreenWidth() || y<0 || y>=ScreenHeight()) {
			return;
		}

		if (depthBuffer[y * ScreenWidth() + x] > distance) {
			depthBuffer[y * ScreenWidth() + x] = distance;
			olc::PixelGameEngine::Draw(x, y, color);
		}
	}

	float dist(float x1, float y1, float x2, float y2) {
		return sqrt(pow(x1 - x2, 2) + pow(y1 - y2, 2));
	}

	bool isEdge(float hitX, float hitY) {
		float fractX = hitX - floor(hitX);
		float fractY = hitY - floor(hitY);

		return fractX < 0.1 || fractX > 0.9 || fractY < 0.1 || fractY > 0.9;
	}

	void drawMap() {
		for (int x = 0; x < W; x++) {
			for (int y = 0; y < H; y++) {
				if (gameMap[y][x] == 1) {
					DrawRect(olc::vi2d(x * 10, y * 10), olc::vi2d(10, 10));
				}
			}
		}

		DrawRect(olc::vi2d(player.x * 10, player.y * 10), olc::vi2d(3, 3), olc::GREEN);

		DrawLine(olc::vi2d(player.x * 10, player.y * 10), olc::vi2d(cosf(player.angle - HFOV)*15 + player.x*10, sinf(player.angle - HFOV)*15 + player.y*10), olc::GREEN);
		DrawLine(olc::vi2d(player.x * 10, player.y * 10), olc::vi2d(cosf(player.angle + HFOV)*15 + player.x*10, sinf(player.angle + HFOV)*15 + player.y*10), olc::GREEN);

		float a = player.angle - HFOV;
		for (int i = 0; i < rayCount; i++) {
			RaycastResult result = shootRay(a);
			if (result.bHit) {
				//cout << "yes " <<  result.distance << ' ' << i << endl;
				DrawLine(olc::vi2d(player.x * 10, player.y * 10), olc::vi2d(cosf(a) * result.distance*10 + player.x * 10, sinf(a) * result.distance*10 + player.y * 10), olc::GREEN);
			}
			a += deltaFOV;
		}
		//DrawCircle()

		for (GameObject* obj : gameObjects) {
			FillCircle(obj->pos*10, 2, olc::RED);
		}
	}

	RaycastResult shootRay(float angle) {
		return cast_ray(olc::vf2d(player.x, player.y), olc::vf2d(cosf(angle), sinf(angle)), gameMap, olc::vi2d(W, H), MAX_DISTANCE);
	}


	void drawWall(int x) {
		float angle = x / ((float)ScreenWidth()) * FOV - HFOV + player.angle;
		olc::vf2d direction(cosf(angle), sinf(angle));
		olc::vf2d rayStart(player.x, player.y);
		RaycastResult ray = cast_ray(rayStart, direction, gameMap, { W,H }, MAX_DISTANCE);
		//std::cout << ray.distance << '|' << angle << "RAy\n";
		float delta = (float)ScreenHeight() / ray.distance / 2;
		int ceiling = (float)ScreenHeight() / 2 - delta;
		int floor = (float)ScreenHeight() / 2 + delta;

		
		for (int y = 0; y < ceiling; y++) {
			Draw(x, y, olc::BLUE, ray.distance);
		}

		olc::vf2d hitPoint = direction * ray.distance + rayStart;
		olc::vi2d blockPoint = hitPoint;
		//olc::vf2d hitRemainder(hitPoint.x - (int)hitPoint.x, hitPoint.y - (int)hitPoint.y);
		float textureOffset;
		float textureAngle = std::atan2(hitPoint.y - blockPoint.y - 0.5f, hitPoint.x - blockPoint.x - 0.5f);
		
		if (textureAngle >= AngleA && textureAngle < AngleB) {
			textureOffset = (hitPoint.x - (int)hitPoint.x);
		}
		else if (textureAngle >= AngleB && textureAngle < AngleC) {
			textureOffset = (hitPoint.y - (int)hitPoint.y);
		}
		else if (textureAngle >= AngleC && textureAngle < AngleD) {
			textureOffset = (hitPoint.x - (int)hitPoint.x);
		}
		else {
			textureOffset = (hitPoint.y - (int)hitPoint.y);
		}

		for (int y = std::max(0,ceiling); y < std::min(ScreenHeight(),floor); y++) {
			olc::Pixel wallColor = wallTexture->Sample(textureOffset, (y - ceiling) / (float)(floor - ceiling));
			Draw(x, y, wallColor, ray.distance);
		}

		for (int y = floor; y < ScreenHeight(); y++) {
			Draw(x, y, olc::DARK_RED, ray.distance);
		}
	}

	void raycast() {
		for (int x = 0; x < ScreenWidth(); x++) {
			drawWall(x);
			//std::cout << x << "DRAWN COL\n";
		}
	}

	void drawObjects() {
		//olc::vf2d playerLookDir = player.getLookDir();
		//olc::vf2d playerPos(player.x, player.y);
		//float pAngleRmdr = fmodf(player.angle, 2*PI);

		for (GameObject* obj : gameObjects) {
			//olc::vf2d pPos(player.x, player.y);
			//float angleToX = std::atan2(obj->pos.x - player.x, obj->pos.y - player.y);
			float angle = -std::atan2f(sinf(player.angle), cosf(player.angle)) + std::atan2f(obj->pos.y - player.y, obj->pos.x - player.x);
			float temp = angle;
			if (angle > PI) {
				angle -= 2 * PI;
			}
			else if (angle < -PI) {
				angle += 2 * PI;
			}
			//std::cout << angle << ' ' << temp << endl;
			
			float distance = (olc::vf2d(player.x, player.y) - obj->pos).mag();
			//float distance = sqrtf(powf(player.x - obj->pos.x, 2) + powf(player.y - obj->pos.y, 2.0f));
			
			if (angle >= -HFOV && angle <= HFOV && distance > 0.5f) {
				float delta = ScreenHeight() / distance / 2 * obj->scale;
				int top = (float)ScreenHeight() / 2 - delta;
				/*int bottom = (float)ScreenHeight() / 2 + delta;*/
				int bottom = ScreenHeight() - top;
				int height = bottom - top;
				float aspectRatio = (float)obj->sprite->width / obj->sprite->height;
				int width = aspectRatio * height;
				// angle = x/ScreenWidth * FOV - HFOV + player.angle
				int midx = (angle + HFOV)/FOV * ScreenWidth();
				int left = midx - width / 2;

				for (int x = 0; x < width; x++) {
					float u = (float)x / width;
					for (int y = 0; y < height; y++) {
						float v = (float)y / height;
						olc::Pixel color = obj->sprite->Sample(u, v);
						if (color.a > 0)
							Draw(left + x, top + y, color, distance);
					}
				}
			}
		}
	}

public:
	Game()
	{
		// Name your application
		sAppName = "Example";
	}

public:
	Player player = { 2,2,0 };

	bool OnUserCreate() override
	{
		//raycast();
		// Called once at the start, so create things here
		wallTexture = new olc::Sprite("wall_texture_adj.JPG");
		fireballTexture = new olc::Sprite("fireball.png");
		lampTexture = new olc::Sprite("lamp_sprite.png");

		gameObjects.push_back(new GameObject(lampTexture, 3, 3));
		depthBuffer = new float[ScreenWidth()*ScreenHeight()];

		return true;
	}

	bool OnUserUpdate(float fElapsedTime) override
	{

		if (GetKey(olc::W).bHeld) {
			player.x += cosf(player.angle) * fElapsedTime;
			player.y += sinf(player.angle) * fElapsedTime;
		}

		if (GetKey(olc::A).bHeld) {
			player.angle -= 0.5 * fElapsedTime;
		}

		if (GetKey(olc::D).bHeld) {
			player.angle += 0.5 * fElapsedTime;
		}

		if (GetKey(olc::S).bHeld) {
			player.x -= cosf(player.angle) * fElapsedTime;
			player.y -= sinf(player.angle) * fElapsedTime;
		}

		if (GetKey(olc::Q).bHeld) {
			player.y -= cosf(player.angle) * fElapsedTime;
			player.x += sinf(player.angle) * fElapsedTime;
		}

		if (GetKey(olc::E).bHeld) {
			player.y += cosf(player.angle) * fElapsedTime;
			player.x -= sinf(player.angle) * fElapsedTime;
		}

		if (GetKey(olc::SPACE).bPressed) {
			Fireball* fireball = new Fireball(fireballTexture, player.x, player.y, cosf(player.angle)*2, sinf(player.angle)*2, gameMap, gameSize);
			fireball->pos.x += fireball->v.x * 0.1f;
			fireball->pos.y += fireball->v.y * 0.1f;
			fireball->scale = 0.3f;
			gameObjects.push_back(fireball);
		}

		for (int i = gameObjects.size() - 1; i >= 0; i--) {
			gameObjects[i]->update(fElapsedTime);
			if (gameObjects[i]->bRemoved) {
				gameObjects.erase(gameObjects.begin() + i);
			}
		}

		fill(depthBuffer, depthBuffer + ScreenWidth()*ScreenHeight(), 1000.0f);

		Clear(olc::BLACK);
		raycast();
		drawObjects();
		drawMap();

	
		return true;
	}
};

int main()
{
	//Game demo;
	//if (demo.Construct(800, 600, 2, 2))
	//	demo.Start();
	Game window;
	if (window.Construct(600, 600, 1, 1)) {
		window.Start();
	}
	return 0;
}

