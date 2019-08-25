#define GL_SILENCE_DEPRECATION
#include <stdlib.h>
#include <math.h>
#include <assert.h>

#include <algorithm>

#define GL_GLEXT_PROTOTYPES
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <OpenGL/glu.h>

#include "viewer.h"
extern "C" {
#include "trackball.h"
}
#include "fluid.h"
#include "spectrum.h"

bool canRunFragmentProgram(const char* programString);	// from FragmentUtils.cpp

	// cube vertices
GLfloat cv[][3] = {
	{1.0f, 1.0f, 1.0f}, {-1.0f, 1.0f, 1.0f}, {-1.0f, -1.0f, 1.0f}, {1.0f, -1.0f, 1.0f},
	{1.0f, 1.0f, -1.0f}, {-1.0f, 1.0f, -1.0f}, {-1.0f, -1.0f, -1.0f}, {1.0f, -1.0f, -1.0f}
};

	// edges have the form edges[n][0][xyz] + t*edges[n][1][xyz]
float edges[12][2][3] = {
	{{1.0f, 1.0f, -1.0f}, {0.0f, 0.0f, 1.0f}},
	{{-1.0f, 1.0f, -1.0f}, {0.0f, 0.0f, 1.0f}},
	{{-1.0f, -1.0f, -1.0f}, {0.0f, 0.0f, 1.0f}},
	{{1.0f, -1.0f, -1.0f}, {0.0f, 0.0f, 1.0f}},

	{{1.0f, -1.0f, 1.0f}, {0.0f, 1.0f, 0.0f}},
	{{-1.0f, -1.0f, 1.0f}, {0.0f, 1.0f, 0.0f}},
	{{-1.0f, -1.0f, -1.0f}, {0.0f, 1.0f, 0.0f}},
	{{1.0f, -1.0f, -1.0f}, {0.0f, 1.0f, 0.0f}},

	{{-1.0f, 1.0f, 1.0f}, {1.0f, 0.0f, 0.0f}},
	{{-1.0f, -1.0f, 1.0f}, {1.0f, 0.0f, 0.0f}},
	{{-1.0f, -1.0f, -1.0f}, {1.0f, 0.0f, 0.0f}},
	{{-1.0f, 1.0f, -1.0f}, {1.0f, 0.0f, 0.0f}}
};

Viewer::Viewer()
{
	trackball(_quat, 0.0, 0.0, 0.0, 0.0);
	_dist = 5.0f;
	_fp = NULL;
	_texture_data = NULL;

	_draw_cube = _draw_slice_outline = false;
	_dispstring = NULL;

	init_GL();

	_light_dir[0] = -1.0f;
	_light_dir[1] = 0.5f;
	_light_dir[2] = 0.0f;
	gen_ray_templ(N+2);
}

Viewer::~Viewer()
{
	if (_texture_data)
		free(_texture_data);
	if (_fp)
		fclose(_fp);
}

void Viewer::init_GL(void)
{
	glEnable(GL_TEXTURE_3D);
	glEnable(GL_BLEND);

	glGenTextures(2, &_txt[0]);

	glActiveTextureARB(GL_TEXTURE0_ARB);
	glBindTexture(GL_TEXTURE_3D, _txt[0]);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S,GL_CLAMP);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T,GL_CLAMP);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R,GL_CLAMP);

	glActiveTextureARB(GL_TEXTURE1_ARB);
	glBindTexture(GL_TEXTURE_3D, _txt[1]);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S,GL_CLAMP);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T,GL_REPEAT);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R,GL_REPEAT);

#define SPEC_WIDTH 256
	unsigned char* data = (unsigned char*) malloc(SPEC_WIDTH*4);
	spectrum(1500, 3000, SPEC_WIDTH, data);

	unsigned char* texture = (unsigned char*)malloc(SPEC_WIDTH*4*16*16);

	for (int i=0;i<16;i++){
	  for (int j=0;j<16;j++) {
	    for (int k=0;k<SPEC_WIDTH;k++) {
	      float intensity = float(i)*1.0/16.0;
	      float density = float(j)*1.0/16.0;
	      float temperatur = float(k)*1.0f/float(SPEC_WIDTH);

	      int index = (j*SPEC_WIDTH*16+i*SPEC_WIDTH+k)*4;

#define FIRE_THRESH 7
	      if (k>=FIRE_THRESH) {
		texture[index] = (unsigned char)(((float)data[k*4]));
		texture[index+1] = (unsigned char)(((float)data[k*4+1]));
		texture[index+2] = (unsigned char)(((float)data[k*4+2]));
#define MAX_FIRE_ALPHA 0.9f
#define FULL_ON_FIRE 100
		texture[index+3] = 255*MAX_FIRE_ALPHA*(
			(k>FULL_ON_FIRE) ? 1.0f : (k-FIRE_THRESH)/((float)FULL_ON_FIRE-FIRE_THRESH));
	      } else {
			texture[index] = 0;texture[index+1] = 0;texture[index+2] = 0;texture[index+3] = 0;
	      }
	    }
	  }
	}
	glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA, SPEC_WIDTH, 16, 16, 0, GL_RGBA, GL_UNSIGNED_BYTE, texture);

	glGenProgramsARB(1, &_prog[0]);
	glEnable(GL_FRAGMENT_PROGRAM_ARB);
	FILE *fp;

	fp = fopen("fire.fp", "rt");
	if (fp) {
		fseek(fp, 0, SEEK_END);
		int size = ftell(fp);
		fseek(fp, 0, SEEK_SET);
		char* text = (char*) malloc(size+1);
		fread(text, 1, size, fp);
		text[size] = 0;

		if (canRunFragmentProgram(text)) {
			glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, _prog[0]);
			glProgramStringARB(GL_FRAGMENT_PROGRAM_ARB,
				GL_PROGRAM_FORMAT_ASCII_ARB, strlen(text), text);
		}
	}
	else
	{
		printf("Error: Unable to open fragment program\n");
	}
}

void Viewer::draw(void)
{
	int i;

	glClearColor(0, 0, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	gluLookAt(0, 0, -_dist,  0, 0, 0,  0, 1, 0);

	float m[4][4];
	build_rotmatrix(m, _quat);

	glMultMatrixf(&m[0][0]);

	if (_draw_cube)
		draw_cube();
	draw_slices(m, _draw_slice_outline);

	if (_dispstring != NULL) {
		glMatrixMode(GL_PROJECTION);
		glLoadMatrixd(_ortho_m);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();

		glDisable(GL_TEXTURE_3D);
		glDisable(GL_FRAGMENT_PROGRAM_ARB);
		glColor4f(1.0, 1.0, 1.0, 1.0);
		glRasterPos2i(-_sx/2 + 10, _sy/2 - 15);

		glMatrixMode(GL_PROJECTION);
		glLoadMatrixd(_persp_m);
		glMatrixMode(GL_MODELVIEW);
	}
}


void Viewer::draw_cube(void)
{
	glDisable(GL_TEXTURE_3D);
	glDisable(GL_FRAGMENT_PROGRAM_ARB);
	glBegin(GL_LINES);
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	glVertex3fv(cv[0]); glVertex3fv(cv[1]);
	glVertex3fv(cv[1]); glVertex3fv(cv[2]);
	glVertex3fv(cv[2]); glVertex3fv(cv[3]);
	glVertex3fv(cv[3]); glVertex3fv(cv[0]);

	glVertex3fv(cv[4]); glVertex3fv(cv[5]);
	glVertex3fv(cv[5]); glVertex3fv(cv[6]);
	glVertex3fv(cv[6]); glVertex3fv(cv[7]);
	glVertex3fv(cv[7]); glVertex3fv(cv[4]);

	glVertex3fv(cv[0]); glVertex3fv(cv[4]);
	glVertex3fv(cv[1]); glVertex3fv(cv[5]);
	glVertex3fv(cv[2]); glVertex3fv(cv[6]);
	glVertex3fv(cv[3]); glVertex3fv(cv[7]);

	glEnd();
}

class Convexcomp
{
private:
	const Vec3 &p0, &up;
public:
	Convexcomp(const Vec3& p0, const Vec3& up) : p0(p0), up(up) {}

	bool operator()(const Vec3& a, const Vec3& b) const
	{
		Vec3 va = a-p0, vb = b-p0;
		return dot(up, cross(va, vb)) >= 0;
	}
};

void Viewer::draw_slices(float m[][4], bool frame)
{
	int i;

	Vec3 viewdir(m[0][2], m[1][2], m[2][2]);
	viewdir.Normalize();
		// find cube vertex that is closest to the viewer
	for (i=0; i<8; i++) {
		float x = cv[i][0] + viewdir[0];
		float y = cv[i][1] + viewdir[1];
		float z = cv[i][2] + viewdir[2];
		if ((x>=-1.0f)&&(x<=1.0f)
			&&(y>=-1.0f)&&(y<=1.0f)
			&&(z>=-1.0f)&&(z<=1.0f))
		{
			break;
		}
	}
	assert(i != 8);

	glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	glDisable(GL_DEPTH_TEST);
		// our slices are defined by the plane equation a*x + b*y +c*z + d = 0
		// (a,b,c), the plane normal, are given by viewdir
		// d is the parameter along the view direction. the first d is given by
		// inserting previously found vertex into the plane equation
	float d0 = -(viewdir[0]*cv[i][0] + viewdir[1]*cv[i][1] + viewdir[2]*cv[i][2]);
	float dd = 2*d0/64.0f;
	int n = 0;
	for (float d = -d0; d < d0; d += dd) {
					// intersect_edges returns the intersection points of all cube edges with
			// the given plane that lie within the cube
		std::vector<Vec3> pt = intersect_edges(viewdir[0], viewdir[1], viewdir[2], d);

		if (pt.size() > 2) {
				// sort points to get a convex polygon
			std::sort(pt.begin()+1, pt.end(), Convexcomp(pt[0], viewdir));

			glEnable(GL_TEXTURE_3D);
			glEnable(GL_FRAGMENT_PROGRAM_ARB);
			glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, _prog[0]);
			glActiveTextureARB(GL_TEXTURE0_ARB);
			glBindTexture(GL_TEXTURE_3D, _txt[0]);
			glBegin(GL_POLYGON);
			for (i=0; i<pt.size(); i++){
			  glColor3f(1.0, 1.0, 1.0);
			  glTexCoord3d((pt[i][0]+1.0)/2.0, (-pt[i][1]+1)/2.0, (pt[i][2]+1.0)/2.0);
			  glVertex3f(pt[i][0], pt[i][1], pt[i][2]);
			}
			glEnd();

			if (frame)
			{
				glDisable(GL_TEXTURE_3D);
				glDisable(GL_FRAGMENT_PROGRAM_ARB);
				glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
				glBegin(GL_POLYGON);
				for (i=0; i<pt.size(); i++) {
					glColor3f(0.0, 0.0, 1.0);
					glVertex3f(pt[i][0], pt[i][1], pt[i][2]);
				}
				glEnd();
				glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			}
		}
		n++;
	}
}

std::vector<Vec3> Viewer::intersect_edges(float a, float b, float c, float d)
{
	int i;
	float t;
	Vec3 p;
	std::vector<Vec3> res;

	for (i=0; i<12; i++) {
		t = -(a*edges[i][0][0] + b*edges[i][0][1] + c*edges[i][0][2] + d)
			/ (a*edges[i][1][0] + b*edges[i][1][1] + c*edges[i][1][2]);
		if ((t>0)&&(t<2)) {
			p[0] = edges[i][0][0] + edges[i][1][0]*t;
			p[1] = edges[i][0][1] + edges[i][1][1]*t;
			p[2] = edges[i][0][2] + edges[i][1][2]*t;
			res.push_back(p);
		}
	}

	return res;
}

void Viewer::viewport(int w, int h)
{
	glViewport(0, 0, w, h);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	GLdouble size = (GLdouble)((w >= h) ? w : h) / 2.0, aspect;
	if (w <= h) {
		aspect = (GLdouble)h/(GLdouble)w;
		glOrtho(-size, size, -size*aspect, size*aspect, -100000.0, 100000.0);
	}
	else {
		aspect = (GLdouble)w/(GLdouble)h;
		glOrtho(-size*aspect, size*aspect, -size, size, -100000.0, 100000.0);
	}
	glScaled(aspect, aspect, 1.0);
	glGetDoublev(GL_PROJECTION_MATRIX, _ortho_m);

	glLoadIdentity();
	gluPerspective(45.0 /* fov */,
		(GLdouble) w/h /* aspect ratio */,
		0.1 /* zNear */,
		100.0 /* zFar */
	);
	glGetDoublev(GL_PROJECTION_MATRIX, _persp_m);

	_sx = w;
	_sy = h;
}

void Viewer::anchor(int x, int y)
{
	_ax = x;
	_ay = y;
}

void Viewer::rotate(int x, int y)
{
    float spin_quat[4];

	float sx2 = _sx*0.5f, sy2 = _sy*0.5f;

	trackball(spin_quat,
		(_ax - sx2) / sx2,
		(_ay - sy2) / sy2,
		(x - sx2) / sx2,
		(y - sy2) / sy2);
	add_quats(spin_quat, _quat, _quat);
	_ax = x;
	_ay = y;
}

void Viewer::dolly(int y)
{
	_dist += ((float)y - _ay)/((float)_sy) * _dist;
	_ay = y;
}

bool Viewer::open(char* filename)
{
	if (_fp)
		fclose(_fp);

	_fp = fopen(filename, "rb");
	if (!_fp)
		return false;

	fread(&_N, sizeof(int), 1, _fp);
	fread(&_N, sizeof(int), 1, _fp);
	printf("Resolution: %dx%dx%d\n", _N, _N, _N);
	if (_texture_data)
		free(_texture_data);
	_texture_data = (unsigned char*) malloc(_N*_N*_N*4);

	fread(&_nframes, sizeof(int), 1, _fp);
	printf("Number of frames: %d\n", _nframes);
	_cur_frame = 0;
}

void Viewer::load_frame(void)
{
	float* tmp = (float*) malloc(_N*_N*_N*sizeof(float));
	float tmin=9999999, tmax=-99999999;

	if (++_cur_frame == _nframes) {
		fseek(_fp, 12, SEEK_SET);
		_cur_frame = 0;
	}

	fread(tmp, sizeof(float), _N*_N*_N, _fp);

	for (int i=0; i<_N*_N*_N; i++)
	{
		_texture_data[(i<<2)+1] = (unsigned char) (tmp[i]*255.0f);
	}

	fread(tmp, sizeof(float), _N*_N*_N, _fp);

	for (int i=0; i<_N*_N*_N; i++)
	{
		_texture_data[(i<<2)] = (unsigned char) (tmp[i]*255.0f);
		if (tmp[i]<tmin)
			tmin = tmp[i];
		if (tmp[i]>tmax)
			tmax = tmp[i];
		_texture_data[(i<<2)+2] = 0;
		_texture_data[(i<<2)+3] = 255;
	}

	cast_light(_N);

	glActiveTextureARB(GL_TEXTURE0_ARB);
	glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA, _N, _N, _N, 0, GL_RGBA, GL_UNSIGNED_BYTE, _texture_data);
}

void Viewer::frame_from_sim(Fluid* fluid)
{
	if (_texture_data == NULL)
		_texture_data = (unsigned char*) malloc((N+2)*(N+2)*(N+2)*4);

	for (int i=0; i<(N+2)*(N+2)*(N+2); i++) {
		_texture_data[(i<<2)] = (unsigned char) (fluid->T[i] * 255.0f);
		_texture_data[(i<<2)+1] = (unsigned char) (fluid->d[i] * 255.0f);
		_texture_data[(i<<2)+2] = 0;
		_texture_data[(i<<2)+3] = 255;
	}

	cast_light(N+2);

	glActiveTextureARB(GL_TEXTURE0_ARB);
	glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA, N+2, N+2, N+2, 0, GL_RGBA, GL_UNSIGNED_BYTE, _texture_data);
}

#define ALMOST_EQUAL(a, b) ((fabs(a-b)<0.00001f)?true:false)
void Viewer::gen_ray_templ(int edgelen)
{
	_ray_templ[0][0] = _ray_templ[0][2] = _ray_templ[0][2] = 0;
	float fx = 0.0f, fy = 0.0f, fz = 0.0f;
	int x = 0, y = 0, z = 0;
	float lx = _light_dir[0] + 0.000001f, ly = _light_dir[1] + 0.000001f, lz = _light_dir[2] + 0.000001f;
	int xinc = (lx > 0) ? 1 : -1;
	int yinc = (ly > 0) ? 1 : -1;
	int zinc = (lz > 0) ? 1 : -1;
	float tx, ty, tz;
	int i = 1;
	int len = 0;
	int maxlen = 3*edgelen*edgelen;
	while (len <= maxlen)
	{
		// fx + t*lx = (x+1)   ->   t = (x+1-fx)/lx
		tx = (x+xinc-fx)/lx;
		ty = (y+yinc-fy)/ly;
		tz = (z+zinc-fz)/lz;

		if ((tx<=ty)&&(tx<=tz)) {
			_ray_templ[i][0] = _ray_templ[i-1][0] + xinc;
			x += xinc;
			fx = x;

			if (ALMOST_EQUAL(ty,tx)) {
				_ray_templ[i][1] = _ray_templ[i-1][1] + yinc;
				y += yinc;
				fy = y;
			} else {
				_ray_templ[i][1] = _ray_templ[i-1][1];
				fy += tx*ly;
			}

			if (ALMOST_EQUAL(tz,tx)) {
				_ray_templ[i][2] = _ray_templ[i-1][2] + zinc;
				z += zinc;
				fz = z;
			} else {
				_ray_templ[i][2] = _ray_templ[i-1][2];
				fz += tx*lz;
			}
		} else if ((ty<tx)&&(ty<=tz)) {
			_ray_templ[i][0] = _ray_templ[i-1][0];
			fx += ty*lx;

			_ray_templ[i][1] = _ray_templ[i-1][1] + yinc;
			y += yinc;
			fy = y;

			if (ALMOST_EQUAL(tz,ty)) {
				_ray_templ[i][2] = _ray_templ[i-1][2] + zinc;
				z += zinc;
				fz = z;
			} else {
				_ray_templ[i][2] = _ray_templ[i-1][2];
				fz += ty*lz;
			}
		} else {
			assert((tz<tx)&&(tz<ty));
			_ray_templ[i][0] = _ray_templ[i-1][0];
			fx += tz*lx;
			_ray_templ[i][1] = _ray_templ[i-1][1];
			fy += tz*ly;
			_ray_templ[i][2] = _ray_templ[i-1][2] + zinc;
			z += zinc;
			fz = z;
		}

		len = _ray_templ[i][0]*_ray_templ[i][0]
			+ _ray_templ[i][1]*_ray_templ[i][1]
			+ _ray_templ[i][2]*_ray_templ[i][2];
		i++;
	}
}

#define DECAY 0.04f
void Viewer::cast_light(int n /*edgelen*/)
{
	int i,j;
	int sx = (_light_dir[0]>0) ? 0 : n-1;
	int sy = (_light_dir[1]>0) ? 0 : n-1;
	int sz = (_light_dir[2]>0) ? 0 : n-1;

	float decay = 1.0f/(n*DECAY);

	for (i=0; i<n; i++)
		for (j=0; j<n; j++) {
			if (!ALMOST_EQUAL(_light_dir[0], 0))
				light_ray(sx,i,j,n,decay);
			if (!ALMOST_EQUAL(_light_dir[1], 0))
				light_ray(i,sy,j,n,decay);
			if (!ALMOST_EQUAL(_light_dir[2], 0))
				light_ray(i,j,sz,n,decay);
		}
}


#define AMBIENT 50
inline void Viewer::light_ray(int x, int y, int z, int n, float decay)
{
	int xx = x, yy = y, zz = z, i = 0;
	int offset;

	int l = 255;
	float d;

	do {
		offset = ((((zz*n) + yy)*n + xx) << 2);
		if (_texture_data[offset + 2] > 0)
			_texture_data[offset + 2] = (unsigned char) ((_texture_data[offset + 2] + l)*0.5f);
		else
			_texture_data[offset + 2] = (unsigned char) l;
		d = _texture_data[offset+1];
		if (l > AMBIENT) {
			l -= d*decay;
			if (l < AMBIENT)
				l = AMBIENT;
		}

		i++;
		xx = x + _ray_templ[i][0];
		yy = y + _ray_templ[i][1];
		zz = z + _ray_templ[i][2];
	} while ((xx>=0)&&(xx<n)&&(yy>=0)&&(yy<n)&&(zz>=0)&&(zz<n));
}

