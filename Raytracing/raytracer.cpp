#include <cstdlib>
#include <cstdio>
#include <cmath>
#include <fstream>
#include <vector>
#include <iostream>
#include <cassert>
#include <algorithm>
#include <chrono>
#include <thread>
#include <utility>
#include <mutex>
#include <iostream>

#include <nanogui/nanogui.h>
#include <nanogui/screen.h>

#include "settings.h"

using namespace nanogui;

#if defined __linux__ || defined __APPLE__
// "Compiled for Linux
#else
	// Windows definiert diese Konstanten Makros standartmäßig nicht
	#define M_PI 3.141592653589793
	#define INFINITY 1e8
	#define srand48(x) srand((int)(x))
#endif

template<typename T>
class Vec3
{
public:
    T x, y, z;
    Vec3() : x(T(0)), y(T(0)), z(T(0)) {}
    Vec3(T xx) : x(xx), y(xx), z(xx) {}
    Vec3(T xx, T yy, T zz) : x(xx), y(yy), z(zz) {}
    Vec3& normalize()
    {
        T nor2 = length2();
        if (nor2 > 0) {
            T invNor = 1 / sqrt(nor2);
            x *= invNor, y *= invNor, z *= invNor;
        }
        return *this;
    }
    Vec3<T> operator * (const T &f) const { return Vec3<T>(x * f, y * f, z * f); }
    Vec3<T> operator * (const Vec3<T> &v) const { return Vec3<T>(x * v.x, y * v.y, z * v.z); }
    T dot(const Vec3<T> &v) const { return x * v.x + y * v.y + z * v.z; }
    Vec3<T> operator - (const Vec3<T> &v) const { return Vec3<T>(x - v.x, y - v.y, z - v.z); }
    Vec3<T> operator + (const Vec3<T> &v) const { return Vec3<T>(x + v.x, y + v.y, z + v.z); }
    Vec3<T>& operator += (const Vec3<T> &v) { x += v.x, y += v.y, z += v.z; return *this; }
    Vec3<T>& operator *= (const Vec3<T> &v) { x *= v.x, y *= v.y, z *= v.z; return *this; }
    Vec3<T> operator - () const { return Vec3<T>(-x, -y, -z); }
    T length2() const { return x * x + y * y + z * z; }
    T length() const { return sqrt(length2()); }
    friend std::ostream & operator << (std::ostream &os, const Vec3<T> &v)
    {
        os << "[" << v.x << " " << v.y << " " << v.z << "]";
        return os;
    }
};

typedef Vec3<float> Vec3f;

class Sphere
{
public:
    Vec3f center;                           /// position der Kugel
    float radius, radius2;                  /// Radius der Kugel und Radius^2
    Vec3f surfaceColor, emissionColor;      /// Oberflächenfarbe- und Leuchtfarbe
    float transparency, reflection;         /// Oberflächentransperenz - und Reflektivität
    Sphere(
        const Vec3f &c,
        const float &r,
        const Vec3f &sc,
        const float &refl = 0,
        const float &transp = 0,
        const Vec3f &ec = 0) :
        center(c), radius(r), radius2(r * r), surfaceColor(sc), emissionColor(ec),
        transparency(transp), reflection(refl)
    { /* empty */ }
    // Berechne Strahl-Kugel Schnittpunkt
    bool intersect(const Vec3f &rayorig, const Vec3f &raydir, float &t0, float &t1) const
    {
        Vec3f l = center - rayorig;
        float tca = l.dot(raydir);
        if (tca < 0) return false;
        float d2 = l.dot(l) - tca * tca;
        if (d2 > radius2) return false;
        float thc = sqrt(radius2 - d2);
        t0 = tca - thc;
        t1 = tca + thc;
        
        return true;
    }
};

float mix(const float &a, const float &b, const float &mix)
{
    return b * mix + a * (1 - mix);
}

double fRand(double fMin, double fMax){
	double f = (double)rand() / RAND_MAX;
	return fMin + f * (fMax - fMin);
}

// Die Haupt-Trace Funktion nutzt ein Strahl, definiert als Ursprung und Richtung, um zu testen
// ob er ein Objekt in der Szene schneidet.
// Falls der Strahl ein Objekt schneidet, wird der Schnittpunkt berechnet, die Normale am Schnittpunkt
// und berechnen mithilfe dieser Informationen die Schattierung an diesem Punkt
// Schatten hängen von den Oberflächen Eigenschaften (transparenz, refliktivität, diffuse) ab.
// Anschließend wird die Farbe des Strahls zurückgegeben. Falls der Strahl ein Objekt schneidet, wird
// ist es die Farbe des Objekts am Schnittpunkt, sonst die  Hintergrundfarbe.
Vec3f trace(
    const Vec3f &rayorig,
    const Vec3f &raydir,
    const std::vector<Sphere> &spheres,
    const int &depth)
{
    float tnear = INFINITY;
    const Sphere* sphere = NULL;
    // Schnittpunkt mit Kugeln der Scene bestimmen
    for (unsigned i = 0; i < spheres.size(); ++i) {
        float t0 = INFINITY, t1 = INFINITY;
        if (spheres[i].intersect(rayorig, raydir, t0, t1)) {
            if (t0 < 0) t0 = t1;
            if (t0 < tnear) {
                tnear = t0;
                sphere = &spheres[i];
            }
        }
    }
    // ist kein Schnittpunkt vorhanden, Hintergrundfarbe oder Schwarz zurück gegeben
    if (!sphere) return Vec3f(2);
    Vec3f surfaceColor = 0; // Farbe des Strahls/Oberfläche mit dem sich der Strahl schneidet
    Vec3f phit = rayorig + raydir * tnear; // Schnittpunkt
    Vec3f nhit = phit - sphere->center; // Normale des Schnittpunkts
    nhit.normalize(); // Normale normalaisieren (länge = 1)
    // Falls Normale und Strahlenrichtung NICHT entgegengesetzt
    // invertieren wir den Normalen Vektor. Das bedeutet allerdings auch, dass wir uns in der Kugel
	// befinden, weshalb 'inside' true wird.
    float bias = 1e-4; // Bias zum Strahlenpunkt hinzufügen
    bool inside = false;
    if (raydir.dot(nhit) > 0) nhit = -nhit, inside = true;
    if ((sphere->transparency > 0 || sphere->reflection > 0) && depth < MAX_RAY_DEPTH) {
        float facingratio = -raydir.dot(nhit);
        // zum Anpassen des Effektes den MIX-Wert ändern
        float fresneleffect = mix(pow(1 - facingratio, 3), 1, 0.1);
        // berechne Reflektions-Richtung (alle Vektoren sind bereits normalisiert)
        Vec3f refldir = raydir - nhit * 2 * raydir.dot(nhit);
        refldir.normalize();
        Vec3f reflection = trace(phit + nhit * bias, refldir, spheres, depth + 1);
        Vec3f refraction = 0;
        // wenn die Kugel transparent ist wird noch der Refraction-Strahl berechnet (transmission)
        if (sphere->transparency) {
            float ior = 1.1, eta = (inside) ? ior : 1 / ior; // Befinden wir uns innerhalb oder außerhalb der Kugel?
            float cosi = -nhit.dot(raydir);
            float k = 1 - eta * eta * (1 - cosi * cosi);
            Vec3f refrdir = raydir * eta + nhit * (eta *  cosi - sqrt(k));
            refrdir.normalize();
            refraction = trace(phit - nhit * bias, refrdir, spheres, depth + 1);
        }
        // Das ergebnis ist ein Mix aus refraction und reflection (falls die Kugel transparent ist)
        surfaceColor = (
            reflection * fresneleffect +
            refraction * (1 - fresneleffect) * sphere->transparency) * sphere->surfaceColor;
    }
    else {
		// es handelt sich um ein diffuses Objekt, wir müssen den Strahl also nicht weiter verfolgen
		for (unsigned i = 0; i < spheres.size(); ++i) {
			if (spheres[i].emissionColor.x > 0) {
				// Es handelt sich um ein Licht
				Vec3f transmission = 1;
				Vec3f lightDirection = spheres[i].center - phit;
				lightDirection.normalize();

				int raysInShadow = 0;

				for (int x = 0; x < SHADOW_RAYS; x++) {
					double minOff = 1.0 - OFFSET_PERCENT;
					double maxOff = 1.0 + OFFSET_PERCENT;
					Vec3f random = Vec3f(fRand(minOff, maxOff), fRand(minOff, maxOff), fRand(minOff, maxOff));
					Vec3f lightDirectionOffset = SHADOW_RAYS > 1.0 ? lightDirection * random : lightDirection;
					//Vec3f lightDirectionOffset = spheres[i]
					lightDirectionOffset.normalize();
					for (unsigned j = 0; j < spheres.size(); ++j) {
						if (i != j) {
							float t0, t1;
							if (spheres[j].intersect(phit + nhit * bias, lightDirectionOffset, t0, t1)) {
								//transmission = 0;
								raysInShadow++;
								break;
							}
						}
					}
				}
				transmission = std::min(1.0, MIN_SHADOW_BRIGHTNESS + (1.0 - (raysInShadow / SHADOW_RAYS)));
				surfaceColor += sphere->surfaceColor * transmission *
					std::max(float(0), nhit.dot(lightDirection)) * spheres[i].emissionColor;
			}
		}
    }
    
    return surfaceColor + sphere->emissionColor;
}

Vec3f *image;
std::vector<Sphere> spheres;
std::vector<std::pair<unsigned, unsigned>> blocks;
std::mutex mtx;

void renderBlock(unsigned left, unsigned top){
    float fov = 30.0, invWidth = 1 / float(width), invHeight = 1 / float(height);
    float aspectratio = width / float(height);
    float angle = tan(M_PI * 0.5 * fov / 180.);

    for (unsigned y = top; y < BLOCK_HEIGHT + top; ++y) {
        for (unsigned x = left; x < BLOCK_WIDTH + left; ++x) {
            float xx = (2 * ((x + 0.5) * invWidth) - 1) * angle * aspectratio;
            float yy = (1 - 2 * ((y + 0.5) * invHeight)) * angle;
            Vec3f raydir(xx, yy, -1);
            raydir.normalize();
            image[y * width + x] = trace(Vec3f(0), raydir, spheres, 0);
            std::this_thread::yield();
        }
    }
}

void threadMain(){
    mtx.lock();
    while(blocks.size() > 0){
        //std::cout << blocks.size() << " blocks remaining" << std::endl;
        unsigned left = blocks.back().first;
        unsigned top = blocks.back().second;
        blocks.pop_back();
        mtx.unlock();
        renderBlock(left, top);
        mtx.lock();
    }
    mtx.unlock();
}

void renderSinglethreaded(){
    float fov = 30.0, invWidth = 1 / float(width), invHeight = 1 / float(height);
    float aspectratio = width / float(height);
    float angle = tan(M_PI * 0.5 * fov / 180.);

    std::cout << "rendering singlethreaded" << std::endl;

    for (unsigned y = 0; y < height; ++y) {
        for (unsigned x = 0; x < width; ++x) {
            float xx = (2 * ((x + 0.5) * invWidth) - 1) * angle * aspectratio;
            float yy = (1 - 2 * ((y + 0.5) * invHeight)) * angle;
            Vec3f raydir(xx, yy, -1);
            raydir.normalize();
            image[y * width + x] = trace(Vec3f(0), raydir, spheres, 0);
        }
    }
}

// Render Funktion. Es wird ein Strahl pro Pixel berechnet und geben dessen Farbe zurück. 
// Trifft der Strahl eine Kugel geben wird die Farbe der Kugel am Schnittpunkt zurückgegeben,
// falls nicht wird die Hintergrundfarbe zurückgegeben.
void render(){
    image = new Vec3f[width * height];
    // Strahlen verfolgen
    if(THREAD_COUNT == 1) renderSinglethreaded();
    else{
        if((width % BLOCK_WIDTH != 0) || (height % BLOCK_HEIGHT != 0)){
            std::cout << "invalid blocksize, check settings.h" << std::endl;
            delete [] image;
            return;
        }

        for(unsigned i = 0; i < width; i += BLOCK_WIDTH){
            for(unsigned j = 0; j < height; j += BLOCK_HEIGHT){
                blocks.push_back(std::make_pair(i, j));   //top left of each block
            }
        }

        std::cout << "split image into blocks, rendering in " << THREAD_COUNT << " threads" << std::endl;

        std::thread threads[THREAD_COUNT];
        for(int i = 0; i < THREAD_COUNT; i++){
            threads[i] = std::thread(threadMain);
        }
        for(int i = 0; i < THREAD_COUNT; i++){
            threads[i].join();
        }
    }

    // Speichere das Ergebnis in die Datei 'result' (flags unter windows behalten!)
    std::ofstream ofs("./result.ppm", std::ios::out | std::ios::binary);
    ofs << "P6\n" << width << " " << height << "\n255\n";
    for (unsigned i = 0; i < width * height; ++i) {
        ofs << (unsigned char)(std::min(float(1), image[i].x) * 255) <<
               (unsigned char)(std::min(float(1), image[i].y) * 255) <<
               (unsigned char)(std::min(float(1), image[i].z) * 255);
    }
    ofs.close();
    delete [] image;
}

Screen *screen = nullptr;
enum test_enum {
	Item1 = 0,
	Item2,
	Item3
};
bool bvar = true;
int maxRayDepth = 12;
int shadowRays = 32;
double rayOffset = 3.5;
std::chrono::high_resolution_clock::time_point t1;
std::chrono::high_resolution_clock::time_point t2;
// In der Main function wird die Scene bestehend aus 5 Kugeln und einem Licht (das auch eine Kugel ist)
// anschließend wird die Szene mit der 'render'-Funktion ausgegeben
int main(int argc, char **argv)
{
	nanogui::init();
    if(THREAD_COUNT <= 0){
        std::cout << "invalid THREAD_COUNT, check settings.h" << std::endl;
        return 0;
    }

	//Init GUI
	screen = new Screen(Vector2i(500, 700), "Raytracer");

	{
		bool enabled = true;
		FormHelper *gui = new FormHelper(screen);
		ref<Window> window = gui->addWindow(Eigen::Vector2i(10, 10), "Settings");
		gui->addVariable("Max. Ray Depth", maxRayDepth)->setSpinnable(true);
		gui->addVariable("Amount of Shadow Rays", shadowRays);
		gui->addVariable("Shadow Ray offset (%)", rayOffset)->setSpinnable(true);

		

		gui->addButton("Render!", []() {
			t1 = std::chrono::high_resolution_clock::now();
			render();
		});
		screen->setVisible(true);
		screen->performLayout();
		window->center();

		nanogui::mainloop();
	}

	nanogui::shutdown();


	std::chrono::high_resolution_clock::time_point t1 = std::chrono::high_resolution_clock::now();
    srand48(13);
    // position, radius, farbe, reflektivität, transparenz, emission color
    spheres.push_back(Sphere(Vec3f( 0.0, -10004, -20), 10000, Vec3f(0.20, 0.20, 0.20), 0, 0.0));
    spheres.push_back(Sphere(Vec3f( 0.0,      0, -20),     4, Vec3f(1.00, 0.32, 0.36), 1, 0.5));
    spheres.push_back(Sphere(Vec3f( 5.0,     -1, -15),     2, Vec3f(0.90, 0.76, 0.46), 1, 0.0));
    spheres.push_back(Sphere(Vec3f( 5.0,      0, -25),     3, Vec3f(0.65, 0.77, 0.97), 1, 0.0));
    spheres.push_back(Sphere(Vec3f(-5.5,      0, -15),     3, Vec3f(0.90, 0.90, 0.90), 1, 0.0));
    // light
    spheres.push_back(Sphere(Vec3f( 0.0,     20, -30),     3, Vec3f(0.00, 0.00, 0.00), 0, 0.0, Vec3f(3)));
    
	std::chrono::high_resolution_clock::time_point t2 = std::chrono::high_resolution_clock::now();
	auto duration = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();
    
	std::cout << "Berechnungszeit: " << (duration/1000000) << "." << (duration%1000000) << "s";
	getchar();
    return 0;
}
