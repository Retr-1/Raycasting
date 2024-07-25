#define OLC_PGE_APPLICATION
#include "olcPixelGameEngine.h"
#include <vector>
using namespace std;

struct Player {
	float x;
	float y;
	float angle; // In radians
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
		{1,0,0,0,1,0,1},
		{1,0,0,0,0,0,1},
		{1,1,1,1,1,1,1},
	};

	int H = gameMap.size();
	int W = gameMap[0].size();

	float FOV = 3.14 / 4;
	float HFOV = FOV / 2;
	float rayCount = 100;
	float deltaFOV = FOV / rayCount;
	float MAX_DISTANCE = 5;

	float dist(float x1, float y1, float x2, float y2) {
		return sqrt(pow(x1 - x2, 2) + pow(y1 - y2, 2));
	}

	bool isEdge(float hitX, float hitY) {
		float fractX = hitX - floor(hitX);
		float fractY = hitY - floor(hitY);

		return fractX < 0.1 || fractX > 0.9 || fractY < 0.1 || fractY > 0.9;
	}



	struct RayInfo { bool hit; float distance; bool edge; } shootRay(Player& player, float angle) {
		float direction[2] = { cosf(angle), sinf(angle) };

		RayInfo result = { false, INFINITY, false };

//		const float step = 0.01;
//		float d = step;
//		while (d < MAX_DISTANCE) {
//			int x = direction[0] * d + player.x;
//			int y = direction[1] * d + player.y;
//			if (x >= W || x < 0 || y >= H || y < 0) return result;
//			if (gameMap[y][x] == 1) {
//				result.distance = d;
//				result.hit = true;
//				return result;
//			}
//;			d += step;
//		}
//
//		return result;

		float line[3] = { direction[1],-direction[0], -direction[1] * player.x + direction[0] * player.y };
		
		//cout << line[0] << ' ' << line[1] << ' ' << line[2] << endl;
		//cout << direction[0] << ' ' << direction[1] << endl;

		

		// Check horizontal intersections
		if (direction[1] > 0) {
			for (float y = ceil(player.y); y <H; y++) {
				float x = (-line[1] * y - line[2]) / line[0];
				raycastLogic(line, x, y, result);
			}
		}
		else {
			for (float y = floor(player.y); y>=0; y--) {
				float x = (-line[1] * y - line[2]) / line[0];
				raycastLogic(line, x, y, result, 0, -1);
			}
		}

		// Check vertical intersect
		if (direction[0] > 0) {
			for (float x = ceil(player.x); x < W; x++) {
				float y = (-line[0] * x - line[2]) / line[1];
				raycastLogic(line, x, y, result);
			}
		}
		else {
			for (float x = floor(player.x); x >= 0; x--) {
				float y = (-line[0] * x - line[2]) / line[1];
				raycastLogic(line, x, y, result, -1);
			}
		}

		return result;
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
			RayInfo result = shootRay(player, a);
			if (result.hit) {
				//cout << "yes " <<  result.distance << ' ' << i << endl;
				DrawLine(olc::vi2d(player.x * 10, player.y * 10), olc::vi2d(cosf(a) * result.distance*10 + player.x * 10, sinf(a) * result.distance*10 + player.y * 10), olc::GREEN);
			}
			a += deltaFOV;
		}
		//DrawCircle()
	}

	void raycastLogic(float line[], float x, float y, RayInfo& result, float offX=0, float offY=0) {
		if (x+offX >= W || x+offX < 0 || y+offY >= H || y+offY < 0) return;
		if (gameMap[(int)y+offY][(int)x+offX] == 1) {
			float d = dist(x, y, player.x, player.y);
			if (d < result.distance) {
				result.distance = d;
				result.hit = true;
				float dx = x - (int)x;
				float dy = y - (int)y;
				result.edge = (dx == 0 && (dy < 0.1 || dy > 0.9)) || (dy == 0 && (dx < 0.1 || dx > 0.9));
			}
		}
	}


	void drawRay(RayInfo& info, int i) {
		//olc::Pixel color = olc::Pixel((1-info.distance / MAX_DISTANCE)*255);
		//auto color = olc::WHITE;
		float perc = max(0.0f, powf((1 - info.distance / MAX_DISTANCE),2));
		olc::Pixel color;
		if (info.edge) {
			color = olc::GREEN;
		}
		else {
			color = olc::Pixel(perc * 255, perc * 255, perc * 255);
		}
		float dx = (float)ScreenWidth() / (float)rayCount;
		float x =  dx * (float) i;

		float halfScreenHeight = ScreenHeight() / 2;
		int halfHeight = perc * halfScreenHeight; //ScreenHeight() - info.distance * 100;//(1 - info.distance / MAX_DISTANCE) * halfScreenHeight;
		FillRect(olc::vi2d(x, halfScreenHeight - halfHeight), olc::vi2d(max(1.0f,dx), halfHeight*2), color);
		//cout << x << ' ' << ScreenHeight() <<  ' ' << rayCount << endl;
		//DrawLine(olc::vi2d(x, 0), olc::vi2d(x, ScreenHeight()));
	}

	void raycast() {
		float a = player.angle - HFOV;
		for (int i = 0; i < rayCount; i++) {
			RayInfo result = shootRay(player, a);
			if (result.hit) {
				//cout << "yes " <<  result.distance << ' ' << i << endl;
				drawRay(result, i);
			}
			a += deltaFOV;
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
		Clear(olc::BLACK);
		raycast();
		drawMap();
	
		return true;
	}
};

int main()
{
	Game demo;
	if (demo.Construct(800, 600, 2, 2))
		demo.Start();
	return 0;
}
