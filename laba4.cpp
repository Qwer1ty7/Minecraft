#include <iostream>
#include <cmath>
#include <string>
#include <audiere.h>
#pragma comment(lib, "audiere.lib")
#include <GL/glut.h>

#include "GL_movement.h"   // здесть удобный класс 3д точки и действия с камерой
#include "readBMP.h"

// меня замучали предупреждения о преобразованиях double во float так что будем игнорировать эти предупреждения

#pragma warning(disable : 4244)  
#pragma warning(disable : 4305)  
#define PI 3.14159265358979323846f

using namespace audiere;
using namespace std;

// ключевое слово extern информирует компиллятор,
// что это внешние переменные и их значение доступно другим файлам
// используется внутри GL_movement

extern GLfloat alX = 0, alY = 0; // угол поворота

								 // начальные данные о положении и совершенном перемещении мыши
extern int lx0 = 0, lx1 = 0, ly0 = 0, ly1 = 0; // левая клавиша
extern int rx0 = 0, rx1 = 0, ry0 = 0, ry1 = 0; // правая клавиша
extern int dveX0 = 0, dveY0 = 0, dveX1 = 0, dveY1 = 0; //две клавиши нажатые вместе

													   /*
													   предопредленый угол зрения (FOV -Field of View) в перспективном режиме
													   или величина приближения(в абсолютных единицах) в ортогональном режиме
													   для приближения камеры(увеличения pzoom) удобно вызывать zoom(+3);
													   для отдаления камеры(уменьшения pzoom) удобно вызывать zoom(-3);
													   */
extern int pzoom = 60;

// тип используемой камеры по умолчанию
// 1 - ортогональная 2-перспективная                        
extern int view = 1;

extern bool m1 = false; // нажата ли левая клавиша мыши
extern bool m2 = false; // нажата ли правая клавиша мыши

						// ининциализация глобавльных переменных
extern GLdouble w = 900, h = 900, px = 0, py = 0;

extern MyPoint e(10, 20, 10);
extern MyPoint c(0, 0, 0);
extern MyPoint u(0, 0, 1);

MyPoint Light_pos(0, 0, 1100);
bool plosk_smooth = true; // сглаживать изображение плоскости?



						  // указатель на текстуру
GLubyte *resImage = 0;
// номер текстуры
GLuint texture1;

// переменная для списка отображения
GLuint Item1;

int resImageWidth, resImageHeight;

int mode = 0;

static void Resize(int width, int height);
static void Draw(void);
void CreateSkyBox(float zfar);
void RenderScene();

void  DrawModel();
double posx = 0, posy = 0, posz = 0;
GLint windowW, windowH;

// функция задания свойст источников освещения
void InitLight(void)
{

	GLfloat amb[] = { 0.1,0.1,0.1,1. };
	GLfloat dif[] = { 1.,1.,1.,1. };
	GLfloat spec[] = { 1.,1.,1.,1. };
	GLfloat pos[] = { Light_pos.x,Light_pos.y,Light_pos.z,1. };

	// параметры источника света
	// позиция источника
	glLightfv(GL_LIGHT0, GL_POSITION, pos);



	// характеристики излучаемого света
	// фоновое освещение (рассеянный свет)
	glLightfv(GL_LIGHT0, GL_AMBIENT, amb);
	// диффузная составляющая света
	glLightfv(GL_LIGHT0, GL_DIFFUSE, dif);
	// зеркально отражаемая составляющая света
	glLightfv(GL_LIGHT0, GL_SPECULAR, spec);

	glDisable(GL_LIGHTING);
	glDisable(GL_BLEND);
	glDisable(GL_TEXTURE_2D);
	glPointSize(10);
	glColor3ub(255, 255, 0);
	glBegin(GL_POINTS);
	glVertex3fv(Light_pos);
	glEnd();
	glEnable(GL_LIGHTING);

}


// функции обработки событий

//extern GLfloat alX=0, alY=0; // угол поворота
int mx0, mx1, my0, my1;

void view_select(void)
{
	view++;
	if (view > 2)
		view = 1;

	pzoom = 80;
	c.Set(0, 0, 0);
	e.Set(10, 20, 10);
	zoom(0);
}

void Light_in_Camera(void)
{
	if (view == 2) // если в перспективе
	{
		Light_pos = e;
	}
}

void plosk_select(void)
{
	plosk_smooth = !plosk_smooth;
}

void loadImage(char *str)
{
	int i, j;
	const char *bmpFileName = str;
	int *srcImage = loadBMP(bmpFileName, resImageWidth, resImageHeight);
	if (!srcImage)
	{
		std::cout << "Could not load an image from the file " << bmpFileName
			<< "\ncheck if the file is present in the working directory.\n";
		return;
	}

	//Выделяем память под наше изображение
	delete[] resImage;
	resImage = new unsigned char[resImageWidth * resImageHeight * 4];

	//Процессим изображение (циклы по линиям и по столбцам)
	for (i = 0; i < resImageHeight; i++)
	{
		for (j = 0; j < resImageWidth; j++)
		{
			//Переприсваиваем в нашем изображении цветовые значения исходного

			int pixelValue = srcImage[i * resImageWidth + j];
			unsigned char red = pixelValue % 256;
			pixelValue >>= 8;
			unsigned char green = pixelValue % 256;
			unsigned char blue = pixelValue >> 8;

			resImage[i*resImageWidth * 4 + j * 4] = red;
			resImage[i*resImageWidth * 4 + j * 4 + 1] = green;
			resImage[i*resImageWidth * 4 + j * 4 + 2] = blue;

			// если цвет их всех равен 0, то сделать альфа компонент =0;
			resImage[i*resImageWidth * 4 + j * 4 + 3] =
				(red || green || blue) ? 255 : 0;
		}
	}
	delete srcImage;
}

void Textura_use(void)
{
	loadImage("List.bmp");

	// устанавливаем формат хранения пикселей
	glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
	//  Команды glGenTextures() и glBindTexture() именуют и создают текстурный объект для изображения текстуры.
	glGenTextures(1, &texture1);
	glBindTexture(GL_TEXTURE_2D, texture1);

	// устанавливаем параметры повторения на краях
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	// параметры отображения текстур если происходит уменьшение или увеличение текстуры
	//    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	//   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	//
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);


	// даем команду на пересылку текстуры в память видеокарты
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, resImageWidth, resImageHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, resImage);


	// способ наложения текстуры GL_MODULATE, GL_DECAL, and GL_BLEND.
	// GL_MODULATE - умножение на цвет от освещения
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_ADD);

	//      glTexEnvf(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_REPLACE);
	glBindTexture(GL_TEXTURE_2D, texture1);

	// создадим два новых списка отображения
	Item1 = glGenLists(2);

	glNewList(Item1, GL_COMPILE);
	glBegin(GL_QUADS);
	glNormal3f(0, 0, 1);

	glTexCoord2f(0, 0);
	glVertex3f(-30, -30, -3);


	glTexCoord2f(1, 0);
	glVertex3f(30, -30, -3);

	glTexCoord2f(1, 1);
	glVertex3f(30, 30, -3);

	glTexCoord2f(0, 1);
	glVertex3f(-30, 30, -3);

	glEnd();
	glEndList();

	MyPoint A(-10, -10, 0), B(-10, 10, 0), C(10, -10, 0), D(10, 10, 0), S(0, 0, 20), N1, N2, N3, N4, T1, T2;

	T1 = B - A;
	T2 = S - A;
	// опереатором *= я переопределил векторное умнежение векторов
	N1 = T2 *= T1;
	N1.Normalize_Self();

	T1 = (D - B);
	T2 = (S - B);
	N2 = T2 *= T1;
	N2.Normalize_Self();

	T1 = (A - C);
	T2 = (S - C);
	N3 = T2 *= T1;
	N3.Normalize_Self();

	T1 = (C - D);
	T2 = (S - D);
	N4 = T2 *= T1;
	N4.Normalize_Self();

	glNewList(Item1 + 1, GL_COMPILE);

	//Рисуем пирамидку
	glBegin(GL_TRIANGLES);

	glNormal3fv(N1);
	glTexCoord2f(1, 1); glVertex3fv(B);
	glTexCoord2f(0, 1); glVertex3fv(A);
	glTexCoord2f(0.5, 0); glVertex3fv(S);

	glNormal3fv(N2);
	glTexCoord2f(1, 1); glVertex3fv(D);
	glTexCoord2f(0, 1); glVertex3fv(B);
	glTexCoord2f(0.5, 0); glVertex3fv(S);

	glNormal3fv(N3);
	glTexCoord2f(1, 1); glVertex3fv(C);
	glTexCoord2f(0, 1); glVertex3fv(D);
	glTexCoord2f(0.5, 0); glVertex3fv(S);

	glNormal3fv(N4);
	glTexCoord2f(1, 1); glVertex3fv(A);
	glTexCoord2f(0, 1); glVertex3fv(C);
	glTexCoord2f(0.5, 0); glVertex3fv(S);

	glEnd();

	glEndList();
	mode = 1;

}

void texture_mode(void)
{
	static bool t = 0;
	if (t)
	{
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	}
	else
	{
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	}
	t = !t;
}

void TextureStart(char *s)
{
	glEnable(GL_TEXTURE_2D);
	// формулу вычисления освещения пикселя см в справке
	loadImage(s);

	// устанавливаем формат хранения пикселей
	glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
	//  Команды glGenTextures() и glBindTexture() именуют и создают текстурный объект для изображения текстуры.
	glGenTextures(1, &texture1);
	glBindTexture(GL_TEXTURE_2D, texture1);

	// устанавливаем параметры повторения на краях
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	// параметры отображения текстур если происходит уменьшение или увеличение текстуры
	//    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	//   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	//
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);


	// даем команду на пересылку текстуры в память видеокарты
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, resImageWidth, resImageHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, resImage);


	// способ наложения текстуры GL_MODULATE, GL_DECAL, and GL_BLEND.
	// GL_MODULATE - умножение на цвет от освещения
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_ADD);

	//      glTexEnvf(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_REPLACE);
	glBindTexture(GL_TEXTURE_2D, texture1);
}

void TextureEnd()
{
	glDeleteTextures(1, &texture1);
	glDisable(GL_TEXTURE_2D);
}

void Key_DOWN(void)
{
	if (view == 1)
		jamp(3, 0);
	else
		strate(3, 0);
}
void Key_UP(void)
{
	if (view == 1)
		jamp(-3, 0);
	else
		strate(-3, 0);
}
void Key_LEFT(void)
{
	jamp(0, 3);
}
void Key_RIGHT(void)
{
	jamp(0, -3);
}

double Ugol = 0, Ugol2 = 0, Ugol3 = 0, Ugol4 = 0, TRAVYSHKA = 0, DOORS = 0;
bool up = true, up2 = true, up3 = true;
double korx = 0, kory = 0, korz = 0, povorot = 0;

// Общий обработчик нажатия клавиш.
void keyPressed(unsigned char key, int x, int y)
{
	switch (key)
	{
	case ' ': view_select(); break;
	case 'w':
	case 'W':
		korx = korx - sin(povorot*(PI / 180));
		kory = kory + cos(povorot*(PI / 180));
		glutPostRedisplay(); break;
	case 's':
	case 'S':
		korx = korx + sin(povorot*(PI / 180));
		kory = kory - cos(povorot*(PI / 180));
		glutPostRedisplay(); break;
	case 'a':
	case 'A':
		kory = kory - cos((povorot - 90)*(PI / 180));
		korx = korx + sin((povorot - 90)*(PI / 180));
		glutPostRedisplay(); break;
	case 'd':
	case 'D':
		kory = kory + cos((povorot - 90)*(PI / 180));
		korx = korx - sin((povorot - 90)*(PI / 180));
		glutPostRedisplay(); break;
	case 'q':
	case 'Q': povorot += 3;
		if (povorot == 360) povorot = 0;
		glutPostRedisplay(); break;
	case 'e':
	case 'E': povorot -= 3;
		if (povorot == -360) povorot = 0;
		glutPostRedisplay(); break;
	case 'L':
	case 'l':
	case '1': Light_in_Camera(); break;
	case 'P':
	case 'p':
	case '2': plosk_select(); break;
	case 'T':
	case 't':
	case '3': Textura_use(); break;
	case '4': texture_mode(); break;
	case 'f': glutFullScreen(); break;
	case 'g': glutReshapeWindow(1280, 720); break;

	case 'h':
	case 'H':
		if (DOORS == 0)
			DOORS = -90;
		else
			DOORS = 0;
		break;
	}
}

void specialKeyPressed(int key, int x, int y)
{
	switch (key)
	{
		//case GLUT_KEY_ESC:
	case GLUT_KEY_DOWN: Key_UP(); break;
	case GLUT_KEY_LEFT: Key_LEFT(); break;
	case GLUT_KEY_RIGHT: Key_RIGHT(); break;
	case GLUT_KEY_UP: Key_DOWN(); break;
		break;
	}

}

void RenderScene()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glLoadIdentity();

	CreateSkyBox(1148.15466);
}

void CreateSkyBox(float zfar)
{
	float dist = zfar * cos(5.0f); //zfar a secas era la distancia del jugador a una de las esquinas de la hitbox
	float th = 1.0f / 3.0f; // un tercio en coordenadas de textura
	float of = (1.0f / 24.0f) / 2; //offset de ~1/2 texel
	TextureStart("skybox.bmp");
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	glBegin(GL_QUADS);
	// Установим текстурные координаты и вершины ПЕРЕДНЕЙ стороны
	glTexCoord2f(0.50f, 2 * th - of); glVertex3f(dist, dist, dist);
	glTexCoord2f(0.25f, 2 * th - of); glVertex3f(-dist, dist, dist);
	glTexCoord2f(0.25f, 1 * th + of); glVertex3f(-dist, dist, -dist);
	glTexCoord2f(0.50f, 1 * th + of); glVertex3f(dist, dist, -dist);
	glEnd();

	glBegin(GL_QUADS);
	// Установим текстурные координаты и вершины ВЕРХНЕЙ стороны
	glTexCoord2f(0.50f - of, 1.0f - of);  glVertex3f(dist, -dist, dist);
	glTexCoord2f(0.50f - of, 2 * th + of);  glVertex3f(-dist, -dist, dist);
	glTexCoord2f(0.25f + of, 2 * th + of);  glVertex3f(-dist, dist, dist);
	glTexCoord2f(0.25f + of, 1.0f - of);  glVertex3f(dist, dist, dist);
	glEnd();

	glBegin(GL_QUADS);
	// Установим текстурные координаты и вершины ЗАДНЕЙ стороны
	glTexCoord2f(0.75f, 1 * th + of); glVertex3f(dist, -dist, -dist);
	glTexCoord2f(1.00f, 1 * th + of); glVertex3f(-dist, -dist, -dist);
	glTexCoord2f(1.00f, 2 * th - of); glVertex3f(-dist, -dist, dist);
	glTexCoord2f(0.75f, 2 * th - of); glVertex3f(dist, -dist, dist);
	glEnd();

	glBegin(GL_QUADS);
	// Установим текстурные координаты и вершины НИЖНЕЙ стороны
	glTexCoord2f(0.50f - of, 1 * th - of); glVertex3f(-dist, -dist, -dist);
	glTexCoord2f(0.25f + of, 1 * th - of); glVertex3f(dist, -dist, -dist);
	glTexCoord2f(0.25f + of, 0.0f + of); glVertex3f(dist, dist, -dist);
	glTexCoord2f(0.50f - of, 0.0f + of); glVertex3f(-dist, dist, -dist);
	glEnd();

	glBegin(GL_QUADS);
	// Установим текстурные координаты и вершины ПРАВОЙ стороны
	glTexCoord2f(0.00f, 2 * th - of); glVertex3f(-dist, -dist, dist);
	glTexCoord2f(0.00f, 1 * th + of); glVertex3f(-dist, -dist, -dist);
	glTexCoord2f(0.25f, 1 * th + of); glVertex3f(-dist, dist, -dist);
	glTexCoord2f(0.25f, 2 * th - of); glVertex3f(-dist, dist, dist);
	glEnd();

	glBegin(GL_QUADS);
	// Установим текстурные координаты и вершины ЛЕВОЙ стороны
	glTexCoord2f(0.75f, 1 * th + of); glVertex3f(dist, -dist, -dist);
	glTexCoord2f(0.75f, 2 * th - of); glVertex3f(dist, -dist, dist);
	glTexCoord2f(0.50f, 2 * th - of); glVertex3f(dist, dist, dist);
	glTexCoord2f(0.50f, 1 * th + of); glVertex3f(dist, dist, -dist);
	glEnd();

	TextureEnd();
}

// действие по нажатию или отпусканию кнопки мыши.
void mouseEvent(int button, int state, int x, int y)
{

	// действие по нажатию левой кнопки мыши
	if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN)
	{
		m1 = true;          // дать пометку, что левая кнопка была нажата
		lx0 = x;            // сохранить х координату мышки
		ly0 = y;            // сохранить y координату мышки

		if ((m1) && (m2))       // если обе(и правая и левая) кнопки мыши были нажаты
		{                       // то сохранить х и y координаты мыши в отдельные переменные,
			dveX0 = x;          //  отвечающие за действие приближения (в орто проекции)  
			dveY0 = y;          //  или  изменения угла зрения в перспективной проекции  

		}

	}
	// действие по событию отпускания левой кнопки мыши
	else if (button == GLUT_LEFT_BUTTON && state == GLUT_UP)
	{
		m1 = false;
	}
	// действие по событию нажатия правой кнопки
	else if (button == GLUT_RIGHT_BUTTON && state == GLUT_DOWN)
	{
		m2 = 1;               // дать пометку, что правая кнопка была нажата
		rx0 = x;              // сохранить х координату мышки
		ry0 = y;              // сохранить y координату мышки

		if ((m1) && (m2))
		{                     // если обе(и правая и левая) кнопки мыши были нажаты
			dveX0 = x;        // то сохранить х и y координаты мыши в отдельные переменные,
			dveY0 = y;        //  отвечающие за действие приближения (в орто проекции)  
		}                     //  или  изменения угла зрения в перспективной проекции 
	}
	// действие по событию отпускания правой кнопки мыши
	else if (button == GLUT_RIGHT_BUTTON && state == GLUT_UP)
	{
		m2 = 0;
	}

}

// общий обработчик вызывающийся при движении мыши
void mouse_move(int x, int y)
{

	if ((m1) && (m2))                                  // а не было ли жвижения мышки с нажатыми обоими кнопками?
	{
		dveX1 = x - dveX0;
		dveY1 = y - dveY0;

		zoom((int)(dveX1 + dveY1));                  // если да то применить приближение/изменения угла зрения

		dveX0 = x;
		dveY0 = y;
		return;
	}
	else
	{
		if (m1)                                        // может была нажата левая кнопка мыши?
		{
			lx1 = x - lx0;
			ly1 = y - ly0;

			jamp(-ly1, lx1);                  // тогда сдвинуться вверх и влево(по перемещению) 

			lx0 = x;
			ly0 = y;
		}
		else if (m2)                                  // или может была нажата правая кнопка мыши?
		{

			rx1 = x - rx0;
			ry1 = y - ry0;

			look_around(-rx1 / 10.0, ry1 / 30.0);              // тогда повернуть камеру

			rx0 = x;
			ry0 = y;
		}
	}

}

void TimerCallback(int fictive)
{
	glutPostRedisplay();
}

int main(int argc, char *argv[])
{
	setlocale(LC_ALL, "Russian");
	// регистрация функций обработки событий от мыши и от клавиатуры
	std::cout
		<< "Active keys:\n"
		<< "[Space bar] - toggle camera mode (parallel or perspective),\n"
		<< "[A] or [a] - Движение Стива влево,\n"
		<< "[D] or [d] - Движение Стива направо,\n"
		<< "[W] or [w] - Движение Стива вперед\n"
		<< "[S] or [s] - Движение Стива Назад\n"
		<< "[Q] or [q] - Круговой стрейф налево\n"
		<< "[E] or [e] - Круговой стрейф направо\n"
		<< "[E] or [e] - Круговой стрейф направо\n"
		<< "[F] or [f] - Войти в полноэкранный режим\n"
		<< "[G] or [g] - Выйти из полноэкранного режима\n"
		<< "[H] or [h] - Открыть/закрыть дверь\n"

		<< "[1] or [l] or [L] - place the light source in the eye position\n"
		<< "                    (works only in the perspective projection mode),\n"
		<< "[2] or [p] or [P] - toggle the brightness model,\n"
		<< "[3] or [t] or [T] - the firtree with textures,\n"
		<< "[4] bilinear filtration on/off.\n"
		<< "\nОБЯЗАТЕЛЬНО ПЕРЕЙДИТЕ В ПЕРСПЕКТИВУ С ПОМОЩЬЮ 'ПРОБЕЛ'!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n"
		<< "\nЕсть 2 трека: track.flac и lol.mp3\n";

	
	//MUSIC
	string filename;
	std::cout << "Enter sound name:";
	std::cin >> filename;
	AudioDevicePtr device = OpenDevice();
	OutputStreamPtr sound(OpenSound(device, filename.c_str(), false));
	sound->play();
	sound->setRepeat(true);
	sound->setVolume(1.0f);
	//

	glutInit(&argc, argv);

	// размер окна OpenGL
	windowW = 1280;
	windowH = 720;

	// расположение окна OpenGL на экране
	glutInitWindowPosition(100, 100);
	glutInitWindowSize(windowW, windowH);

	// инициализация окна OpenGL с заголовком Title
	glutCreateWindow("Minecampf");

	glutMouseFunc(mouseEvent);

	// для события смещения - общий обработчик и при ненажатых кнопках, и при нажатых.
	// один для нажатия правой, левой и одновременно обоих нажатых кнопок
	glutMotionFunc(mouse_move);
	glutPassiveMotionFunc(mouse_move);

	glutKeyboardFunc(keyPressed);
	glutSpecialFunc(specialKeyPressed);


	// установка основных параметров работы OpenGL
	// цветовой режим RGB | включение Z-буфера для сортировки по глубине

	glutInitDisplayMode(GLUT_RGB | GLUT_DEPTH | GLUT_DOUBLE);

	// Инициализация источников света
	// если вне видового преобразования, то источник ориентирован относительно 
	// наблюдателя, т.е. он будет фиксирован в системе координат, 
	// XOY которой связана с экраном. 
	// В нашем примере источник с pos[] = {0.,0.,1.,0.}; светит перпендикулярно экрану
	InitLight();

	// регистрация функции, которая вызывается при изменении размеров окна
	//  Resize() - функция пользователя
	glutReshapeFunc(Resize);

	// регистрация функции, которая вызывается при перерисовке 
	// и запуск цикла обработки событий
	// Draw() - функция пользователя
	gluPerspective(45.0f, (GLfloat)windowW / (GLfloat)windowH, .5f, 500.0f);
	glutDisplayFunc(Draw);
	glutMainLoop();
	return 0;
}



static void Resize(int width, int height) // создается пользователем
{
	//1. получение текущих координат окна 


	w = width;
	h = height;
	// сохраняем размеры окна вывода w,h  
	if (h == 0)
		h = 1;
	// установка новых размеров окна вывода
	glViewport(0, 0, w, h);


	// в ней мы установим тип проекции (ортогональная или перспективная)
	zoom(0);

}



static void Draw(void) // создается пользователем
{
	// 1. очистка буферов (цвета, глубины и возможно др.)

	// установка цвета фона
	glClearColor(0.75f, 0.75f, 0.75f, 1.0f);

	// очистка всех буферов
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	//2.установка режимов рисования

	// нормализация нормалей : их длины будет равна 1
	glEnable(GL_NORMALIZE);

	// включение режима сортировки по глубине
	glEnable(GL_DEPTH_TEST);



	glEnable(GL_LINE_SMOOTH); // устранение ступенчатости для линий
	glEnable(GL_POINT_SMOOTH);

	// для качественного устранения ступенчатости 
	// нам надо включить режим альфа-наложения(альфа смешения BLEND - смешивать)
	glEnable(GL_BLEND);

	// Настройка альфа сглаживания:
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// тип рисования полигонов
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	// первый параметр: рисовать все грани, или только внешние или внутренние
	// GL_FRONT_AND_BACK , GL_FRONT , GL_BACK  
	// второй: как рисовать 
	// GL_POINT (точки) GL_LINE(линии на границе), GL_FILL(заполнять)


	//
	// тип закраски полигонов
	glShadeModel(GL_FLAT);
	// GL_FLAT все пикселы полигона имеюют одинаковый цвет ( за цвет принимается 
	// цвет первой обрабатываемой вершины, если включены источники, 
	// то этот цвет модифицируется с учетом источников и нормали в вершине)
	// GL_SMOOTH цвет каждого пиксела рассчитывается интерполяцией цветов вершин 
	// с учетом источников света. если они включены

	//  включить режим учета освещения
	glEnable(GL_LIGHTING);

	//   задать параметры освещения
	//  параметр GL_LIGHT_MODEL_TWO_SIDE - 
	//                0 -  лицевые и изнаночные рисуются одинаково(по умолчанию), 
	//                1 - лицевые и изнаночные обрабатываются разными режимами       
	//                соответственно лицевым и изнаночным свойствам материалов.    
	//  параметр GL_LIGHT_MODEL_AMBIENT - задать фоновое освещение, 
	//                не зависящее от сточников
	// по умолчанию (0.2, 0.2, 0.2, 1.0)

	glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, 0);


	//  включить нужное количество источников 
	glEnable(GL_LIGHT0);

	// new!  автоматического задания свойств материала 
	// функцией glColor() не используем!!!
	// glEnable(GL_COLOR_MATERIAL);
	// glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

	// 3.установка видового преобразования

	// выбор видовой матрицы в качестве текущей

	glMatrixMode(GL_MODELVIEW);

	// Сохранение текущего значения матрицы в стеке
	//glPushMatrix();

	// загрузка единичной матрицы
	glLoadIdentity();

	// установка точки наблюдения

	gluLookAt(e.x, e.y, e.z, c.x, c.y, c.z, u.x, u.y, u.z);

	InitLight();
	// если здесь инициализировать источник света
	// то он будет зафиксирован в мировых координатах, т.е.
	// при poition(0.,0.,1.,0.) он  будет светить вдоль
	//  мировой оси Z ( в нашем примере синяя ось)

	// преобразования объектов
	// (угол поворота, меняется машью или клавиатурой)
	// glRotatef(alX,1.0f,0.0f, 0.0f);
	// glRotatef(alY,0.0f,1.0f, 0.0f);

	//new!  InitLight();
	// если здесь инициализировать источник, то он будет вращается 
	//вместе с объектом. В нашем случае при позиции источника 
	// pos[] = {0.,0.,1.,0.} при любых вращениях ярко освещена будет 
	//только синяя грань

	// 4. вызов модели
	//RenderScene();
	DrawModel();


	// 5. завершение видового преобразования
	// Restore the view matrix
	//  glPopMatrix();

	//6. Завершить все вызванные на данный момент операции OpenGL
	glFinish();
	glutSwapBuffers();
	int delay = 0;
	if ((!m1) && (!m2))
		delay = 100;
	glutTimerFunc(delay, TimerCallback, 0);
}


MyPoint Norm(double xo, double yo, double zo, double xx, double yy, double zz, double xxx, double yyy, double zzz)
{
	MyPoint O = MyPoint(xo, yo, zo);
	MyPoint T1 = MyPoint(xx, yy, zz);
	MyPoint T2 = MyPoint(xxx, yyy, zzz);
	MyPoint V1 = O - T1;
	MyPoint V2 = T2 - O;
	MyPoint N = V1 *= V2;
	N.Normalize_Self();
	return N;
}


void Cilinder(double xc, double yc, double R, double h, char* tname, bool flipped = true, int shag = 50)
{
	MyPoint N = MyPoint(0, 0, 1);
	//Cilinder(0, 0, 10, 2, "TRAVYSHKA.bmp", true, 10);
	double fs = 0;
	double ff = 2 * 3.149;
	double delta = (ff - fs) / shag;
	double f = fs;

	double xp = xc + R * cos(f);
	double yp = yc + R * sin(f);
	double xi, yi;


	double xpt = 0.5 + 0.5*cos(f);
	double ypt = 0.5 + 0.5*sin(f);
	double xit, yit;

	f = fs + delta;
	for (int i = 0; i < shag; i++, f = f + delta)
	{
		xi = xc + R * cos(f);
		yi = yc + R * sin(f);

		xit = 0.5 + 0.5*cos(f);
		yit = 0.5 + 0.5*sin(f);


		TextureStart(tname);
		glBegin(GL_TRIANGLES);
		glTexCoord2f(0.5, 0.5); glVertex3f(xc, yc, 1);
		glTexCoord2f(xpt, ypt); glVertex3f(xp, yp, 1);
		glTexCoord2f(xit, yit); glVertex3f(xi, yi, 1);
		glEnd();
		TextureEnd();

		xp = xi;
		yp = yi;

		xpt = xit;
		ypt = yit;
	}


	//xi = xc + 2 * cos(f);
	//yi = yc + 2 * sin(f);
}



void MyUpd()
{

	/*glTranslatef(0, kory, 0);
	if (up3)
		kory += 1;
	else
		kory -= 1;*/

		/*if (kory >= 90)
			up3 = false;
		if (kory <= -90)
			up3 = true;*/


			/*glTranslatef(0, 0, korz);
			if (up3)
				korz += 1;
			else
				korz -= 1;*/



	if (up)
	{
		Ugol -= 3;
		Ugol2 += 3;
	}
	else
	{
		Ugol += 3;
		Ugol2 -= 3;
	}
	if (Ugol <= -21 && Ugol2 >= 21)
		up = false;
	if (Ugol >= 21 && Ugol2 <= -21)
		up = true;

	if (up2)
	{
		Ugol3 -= 3;
		Ugol4 += 3;
	}
	else
	{
		Ugol3 += 3;
		Ugol4 -= 3;
	}
	if (Ugol3 <= 21 && Ugol4 >= 21)
		up2 = false;
	if (Ugol3 >= 21 && Ugol4 <= -21)
		up2 = true;

	if (up3)
		TRAVYSHKA -= 1;
	else
		TRAVYSHKA += 1;
	if (TRAVYSHKA <= -6)
		up3 = false;
	if (TRAVYSHKA >= 6)
		up3 = true;
}

void DrawModel()
{
	// геометрическая модель включает шар и
	// 3 отрезка вдоль координатных осей

	/*glColor3f(0.5f, 0.5f, 0.5f);

	GLfloat amb[] = { 0.2,0.2,0.4,1. };
	GLfloat dif[] = { 0.4,0.65,0.5,1. };
	GLfloat spec[] = { 0.9,0.8,0.3,1. };
	GLfloat sh = 0.1f * 256;


	glMaterialfv(GL_FRONT, GL_AMBIENT, amb);
	glMaterialfv(GL_FRONT, GL_DIFFUSE, dif);
	glMaterialfv(GL_FRONT, GL_SPECULAR, spec);
	glMaterialf(GL_FRONT, GL_SHININESS, sh);*/

	/*glDisable(GL_BLEND);
	glShadeModel(GL_SMOOTH);
	glutSolidSphere(1.0, 100, 100);
	glShadeModel(GL_FLAT) ;
	glutSolidTorus (0.5,1.5,100,100);*/

	if (plosk_smooth)
		glShadeModel(GL_SMOOTH);
	else
		glShadeModel(GL_FLAT);
	//Ground

	glEnable(GL_LIGHTING);

	/*if(mode)
	{ glEnable(GL_TEXTURE_2D);
	// рисуем прямоугольник
	// формулу вычисления освещения пикселя см в справке
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
	glCallList(Item1);
	glTranslatef(-15,-15,0);
	// рисуем пирамидку
	// цвет текстуры заменяет освещенность
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	glCallList(Item1+1);

	glEnable(GL_BLEND);
	// настройти приемника и источника
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	glPushMatrix();
	int i;
	glTranslatef(30,0,0);

	// цвет текстуры умножается модифицируется интенсивностью освещения
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	// елочка рисуется 6 раз вывается пирамидка
	for(i=0;i<=6;i++)
	{
	glScalef(1.1-i*0.1,1.1-i*0.1,0.8);
	glTranslatef(0,0,5*i);
	glCallList(Item1+1);
	}
	glPopMatrix();

	}*/

	//Ground
	{
		TextureStart("TRAVA.bmp");
		glBegin(GL_QUADS);
		glTexCoord2f(20, 20); glVertex3f(-150, -150, 0);
		glTexCoord2f(0, 20); glVertex3f(-150, 150, 0);
		glTexCoord2f(0, 0); glVertex3f(150, 150, 0);
		glTexCoord2f(20, 0); glVertex3f(150, -150, 0);
		glEnd();
		TextureEnd();
	}


	//STIVE
	CreateSkyBox(500.15466);
	MyUpd();
	glPushMatrix();
	{
		glTranslatef(korx, kory, 0);
		glRotatef(Ugol, 0, 0, 0);
		glRotatef(povorot, 0, 0, 1);
		glPushMatrix();
		{

			glTranslatef(0, 0, 12);
			glRotatef(180, 0, 1, 0);
			glTranslatef(-22, 0, 0);
			glRotatef(Ugol, 1, 0, 0);

			{
				TextureStart("STOPA.bmp");
				glBegin(GL_POLYGON);
				glNormal3f(0, 0, -1);
				glTexCoord2f(1, 1); glVertex3f(9, 6, 10);
				glTexCoord2f(0, 1); glVertex3f(9, 2, 10);
				glTexCoord2f(0, 0); glVertex3f(13, 2, 10);
				glTexCoord2f(1, 0); glVertex3f(13, 6, 10);
				glEnd();
				TextureEnd();
			}


			glBegin(GL_POLYGON);
			glNormal3f(0, 0, 1);
			glTexCoord2f(0, 1); glVertex3f(9, 6, 2);
			glTexCoord2f(0, 0); glVertex3f(9, 2, 2);
			glTexCoord2f(1, 0); glVertex3f(13, 2, 2);
			glTexCoord2f(1, 1); glVertex3f(13, 6, 2);
			glEnd();

			{
				TextureStart("PRAVII_BOK_NOGI.bmp");
				glBegin(GL_QUADS); //AB
				glNormal3f(-2, 0, 1);
				glTexCoord2f(1, 0); glVertex3f(9, 2, 10);
				glTexCoord2f(1, 1); glVertex3f(9, 2, 2);
				glTexCoord2f(0, 1); glVertex3f(9, 6, 2);
				glTexCoord2f(0, 0); glVertex3f(9, 6, 10);
				glEnd();
				TextureEnd();
			}

			{
				TextureStart("NOGA.bmp");
				glBegin(GL_QUADS);//BC
				glNormal3f(0, 5, 1);
				glTexCoord2f(0, 0); glVertex3f(9, 6, 10);
				glTexCoord2f(0, 1); glVertex3f(9, 6, 2);
				glTexCoord2f(1, 1); glVertex3f(13, 6, 2);
				glTexCoord2f(1, 0); glVertex3f(13, 6, 10);
				glEnd();
				TextureEnd();
			}

			{
				TextureStart("LEVII_BOK_NOGI.bmp");
				glBegin(GL_QUADS);//CD
				glNormal3f(2, 0, 1);
				glTexCoord2f(0, 0); glVertex3f(13, 2, 10);
				glTexCoord2f(0, 1); glVertex3f(13, 2, 2);
				glTexCoord2f(1, 1); glVertex3f(13, 6, 2);
				glTexCoord2f(1, 0); glVertex3f(13, 6, 10);
				glEnd();
				TextureEnd();
			}

			{
				TextureStart("ZADHIYA_GRAN_NOGI.bmp");
				glBegin(GL_QUADS);//DA
				glNormal3f(0, -5, 1);
				glTexCoord2f(1, 0); glVertex3f(13, 2, 10);
				glTexCoord2f(1, 1); glVertex3f(13, 2, 2);
				glTexCoord2f(0, 1); glVertex3f(9, 2, 2);
				glTexCoord2f(0, 0); glVertex3f(9, 2, 10);
				glEnd();
				TextureEnd();
			}

		}
		glPopMatrix();
		//-------------------------------------------------------------------------------------------------------------------------------------------
		//----------------------------------------------------------------------------ЛЕВАЯ НОГА----------------------------------------------------

		glPushMatrix();
		{
			glTranslatef(0, 0, 12);
			glRotatef(180, 0, 1, 0);
			glTranslatef(-32, 0, 0);
			glRotatef(Ugol2, 1, 0, 0);
			{
				TextureStart("STOPA.bmp");
				glBegin(GL_POLYGON);
				glNormal3f(0, 0, -1);
				glTexCoord2f(1, 1); glVertex3f(14, 6, 10);
				glTexCoord2f(0, 1); glVertex3f(14, 2, 10);
				glTexCoord2f(0, 0); glVertex3f(18, 2, 10);
				glTexCoord2f(1, 0); glVertex3f(18, 6, 10);
				glEnd();
				TextureEnd();
			}

			glBegin(GL_POLYGON);
			glNormal3f(0, 0, 1);
			glTexCoord2f(0, 1); glVertex3f(14, 6, 2);
			glTexCoord2f(0, 0); glVertex3f(14, 2, 2);
			glTexCoord2f(1, 0); glVertex3f(18, 2, 2);
			glTexCoord2f(1, 1); glVertex3f(18, 6, 2);
			glEnd();

			{
				TextureStart("PRAVII_BOK_NOGI.bmp");
				glBegin(GL_QUADS); //AB
				glNormal3f(-2, 0, 1);
				glTexCoord2f(1, 0); glVertex3f(14, 2, 10);
				glTexCoord2f(1, 1); glVertex3f(14, 2, 2);
				glTexCoord2f(0, 1); glVertex3f(14, 6, 2);
				glTexCoord2f(0, 0); glVertex3f(14, 6, 10);
				glEnd();
				TextureEnd();
			}

			{
				TextureStart("NOGA.bmp");
				glBegin(GL_QUADS);//BC
				glNormal3f(0, 5, 1);
				glTexCoord2f(0, 0); glVertex3f(14, 6, 10);
				glTexCoord2f(0, 1); glVertex3f(14, 6, 2);
				glTexCoord2f(1, 1); glVertex3f(18, 6, 2);
				glTexCoord2f(1, 0); glVertex3f(18, 6, 10);
				glEnd();
				TextureEnd();
			}

			{
				TextureStart("LEVII_BOK_NOGI.bmp");
				glBegin(GL_QUADS);//CD
				glNormal3f(2, 0, 1);
				glTexCoord2f(0, 0); glVertex3f(18, 2, 10);
				glTexCoord2f(0, 1); glVertex3f(18, 2, 2);
				glTexCoord2f(1, 1); glVertex3f(18, 6, 2);
				glTexCoord2f(1, 0); glVertex3f(18, 6, 10);
				glEnd();
				TextureEnd();
			}

			{
				TextureStart("ZADHIYA_GRAN_NOGI.bmp");
				glBegin(GL_QUADS);//DA
				glNormal3f(0, -5, 1);
				glTexCoord2f(1, 0); glVertex3f(18, 2, 10);
				glTexCoord2f(1, 1); glVertex3f(18, 2, 2);
				glTexCoord2f(0, 1); glVertex3f(14, 2, 2);
				glTexCoord2f(0, 0); glVertex3f(14, 2, 10);
				glEnd();
				TextureEnd();
			}
		}
		glPopMatrix();
		//--------------------------------------------------------------------------------------------------------------------------
	//---------------------------------------------------------ТЕЛО----------------------------------------------------------------
		{
			TextureStart("DNO_TELA.bmp");
			glBegin(GL_POLYGON);
			glNormal3f(0, 0, -1);
			glTexCoord2f(1, 1); glVertex3f(9, 6, 10);
			glTexCoord2f(0, 1); glVertex3f(9, 2, 10);
			glTexCoord2f(0, 0); glVertex3f(18, 2, 10);
			glTexCoord2f(1, 0); glVertex3f(18, 6, 10);
			glEnd();
			TextureEnd();
		}

		{
			TextureStart("PLECHI.bmp");
			glBegin(GL_POLYGON);
			glNormal3f(0, 0, 1);
			glTexCoord2f(0, 1); glVertex3f(9, 6, 20);
			glTexCoord2f(0, 0); glVertex3f(9, 2, 20);
			glTexCoord2f(1, 0); glVertex3f(18, 2, 20);
			glTexCoord2f(1, 1); glVertex3f(18, 6, 20);
			glEnd();
			TextureEnd();
		}

		{
			TextureStart("PRAVII_BOK.bmp");
			glBegin(GL_QUADS); //AB
			glNormal3f(-2, 0, 1);
			glTexCoord2f(1, 0); glVertex3f(9, 2, 10);
			glTexCoord2f(1, 1); glVertex3f(9, 2, 20);
			glTexCoord2f(0, 1); glVertex3f(9, 6, 20);
			glTexCoord2f(0, 0); glVertex3f(9, 6, 10);
			glEnd();
			TextureEnd();
		}

		{
			TextureStart("TELO_PERED.bmp");
			glBegin(GL_QUADS);//BC
			glNormal3f(0, 5, 1);
			glTexCoord2f(1, 0); glVertex3f(9, 6, 10);
			glTexCoord2f(1, 1); glVertex3f(9, 6, 20);
			glTexCoord2f(0, 1); glVertex3f(18, 6, 20);
			glTexCoord2f(0, 0); glVertex3f(18, 6, 10);
			glEnd();
			TextureEnd();
		}

		{
			TextureStart("LEVII_BOK.bmp");
			glBegin(GL_QUADS);//CD
			glNormal3f(2, 0, 1);
			glTexCoord2f(0, 0); glVertex3f(18, 2, 10);
			glTexCoord2f(0, 1); glVertex3f(18, 2, 20);
			glTexCoord2f(1, 1); glVertex3f(18, 6, 20);
			glTexCoord2f(1, 0); glVertex3f(18, 6, 10);
			glEnd();
			TextureEnd();
		}

		{
			TextureStart("CPINA.bmp");
			glBegin(GL_QUADS);//DA
			glNormal3f(0, -5, 1);
			glTexCoord2f(1, 0); glVertex3f(18, 2, 10);
			glTexCoord2f(1, 1); glVertex3f(18, 2, 20);
			glTexCoord2f(0, 1); glVertex3f(9, 2, 20);
			glTexCoord2f(0, 0); glVertex3f(9, 2, 10);
			glEnd();
			TextureEnd();
		}
		//-----------------------------------------------------------------------------------------------------------
		//----------------------------------------------------------ГОЛОВА-------------------------------------------
		{
			TextureStart("SHEYA.bmp");
			glBegin(GL_POLYGON);
			glNormal3f(0, 0, -1);
			glTexCoord2f(1, 1); glVertex3f(9.5, 7.5, 20);
			glTexCoord2f(0, 1); glVertex3f(9.5, 0.5, 20);
			glTexCoord2f(0, 0); glVertex3f(17.5, 0.5, 20);
			glTexCoord2f(1, 0); glVertex3f(17.5, 7.5, 20);
			glEnd();
			TextureEnd();
		}

		{
			TextureStart("MAKYWKA.bmp");
			glBegin(GL_POLYGON);
			glNormal3f(0, 0, 1);
			glTexCoord2f(0, 1); glVertex3f(9.5, 7.5, 28);
			glTexCoord2f(0, 0); glVertex3f(9.5, 0.5, 28);
			glTexCoord2f(1, 0); glVertex3f(17.5, 0.5, 28);
			glTexCoord2f(1, 1); glVertex3f(17.5, 7.5, 28);
			glEnd();
			TextureEnd();
		}

		{
			TextureStart("HEAD_PRAVII_BOK.bmp");
			glBegin(GL_QUADS); //AB
			glNormal3f(-2, 0, 1);
			glTexCoord2f(1, 0); glVertex3f(9.5, 0.5, 20);
			glTexCoord2f(1, 1); glVertex3f(9.5, 0.5, 28);
			glTexCoord2f(0, 1); glVertex3f(9.5, 7.5, 28);
			glTexCoord2f(0, 0); glVertex3f(9.5, 7.5, 20);
			glEnd();
			TextureEnd();
		}

		{
			TextureStart("LICO.bmp");
			glBegin(GL_QUADS);//BC
			glNormal3f(0, 5, 1);
			glTexCoord2f(1, 0); glVertex3f(9.5, 7.5, 20);
			glTexCoord2f(1, 1); glVertex3f(9.5, 7.5, 28);
			glTexCoord2f(0, 1); glVertex3f(17.5, 7.5, 28);
			glTexCoord2f(0, 0); glVertex3f(17.5, 7.5, 20);
			glEnd();
			TextureEnd();
		}

		{
			TextureStart("HEAD_LEVII_BOK.bmp");
			glBegin(GL_QUADS);//CD
			glNormal3f(2, 0, 1);
			glTexCoord2f(0, 0); glVertex3f(17.5, 0.5, 20);
			glTexCoord2f(0, 1); glVertex3f(17.5, 0.5, 28);
			glTexCoord2f(1, 1); glVertex3f(17.5, 7.5, 28);
			glTexCoord2f(1, 0); glVertex3f(17.5, 7.5, 20);
			glEnd();
			TextureEnd();
		}

		{
			TextureStart("ZATILOK.bmp");
			glBegin(GL_QUADS);//DA
			glNormal3f(0, -5, 1);
			glTexCoord2f(1, 0); glVertex3f(17.5, 0.5, 20);
			glTexCoord2f(1, 1); glVertex3f(17.5, 0.5, 28);
			glTexCoord2f(0, 1); glVertex3f(9.5, 0.5, 28);
			glTexCoord2f(0, 0); glVertex3f(9.5, 0.5, 20);
			glEnd();
			TextureEnd();
		}
		//--------------------------------------------------------------------------------------------------------------------------------------------
		//----------------------------------------------------------------ЛЕВАЯ РУКА------------------------------------------------------------------
		glPushMatrix();
		{
			glTranslatef(0, 0, 30);
			glRotatef(180, 0, 1, 0);
			glTranslatef(-27, 0, 0);
			glRotatef(Ugol4, 1, 0, 0);
			{
				TextureStart("LADOWKI.bmp");
				glBegin(GL_POLYGON);
				glNormal3f(0, 0, -1);
				glTexCoord2f(1, 1); glVertex3f(18, 6, 20);
				glTexCoord2f(0, 1); glVertex3f(18, 2, 20);
				glTexCoord2f(0, 0); glVertex3f(22, 2, 20);
				glTexCoord2f(1, 0); glVertex3f(22, 6, 20);
				glEnd();
				TextureEnd();
			}

			{
				TextureStart("PLECHIRYK.bmp");
				glBegin(GL_POLYGON);
				glNormal3f(0, 0, 1);
				glTexCoord2f(0, 1); glVertex3f(18, 6, 10);
				glTexCoord2f(0, 0); glVertex3f(18, 2, 10);
				glTexCoord2f(1, 0); glVertex3f(22, 2, 10);
				glTexCoord2f(1, 1); glVertex3f(22, 6, 10);
				glEnd();
				TextureEnd();
			}

			{
				TextureStart("RYKI_BAZUKI.bmp");
				glBegin(GL_QUADS); //AB
				glNormal3f(-2, 0, 1);
				glTexCoord2f(1, 0); glVertex3f(18, 2, 20);
				glTexCoord2f(1, 1); glVertex3f(18, 2, 10);
				glTexCoord2f(0, 1); glVertex3f(18, 6, 10);
				glTexCoord2f(0, 0); glVertex3f(18, 6, 20);
				glEnd();
				TextureEnd();
			}

			{
				TextureStart("RYKI_BAZUKI.bmp");
				glBegin(GL_QUADS);//BC
				glNormal3f(0, 5, 1);
				glTexCoord2f(1, 0); glVertex3f(18, 6, 20);
				glTexCoord2f(1, 1); glVertex3f(18, 6, 10);
				glTexCoord2f(0, 1); glVertex3f(22, 6, 10);
				glTexCoord2f(0, 0); glVertex3f(22, 6, 20);
				glEnd();
				TextureEnd();
			}

			{
				TextureStart("RYKI_BAZUKI.bmp");
				glBegin(GL_QUADS);//CD
				glNormal3f(2, 0, 1);
				glTexCoord2f(0, 0); glVertex3f(22, 2, 20);
				glTexCoord2f(0, 1); glVertex3f(22, 2, 10);
				glTexCoord2f(1, 1); glVertex3f(22, 6, 10);
				glTexCoord2f(1, 0); glVertex3f(22, 6, 20);
				glEnd();
				TextureEnd();
			}

			{
				TextureStart("RYKI_BAZUKI.bmp");
				glBegin(GL_QUADS);//DA
				glNormal3f(0, -5, 1);
				glTexCoord2f(1, 0); glVertex3f(22, 2, 20);
				glTexCoord2f(1, 1); glVertex3f(22, 2, 10);
				glTexCoord2f(0, 1); glVertex3f(18, 2, 10);
				glTexCoord2f(0, 0); glVertex3f(18, 2, 20);
				glEnd();
				TextureEnd();
			}
		}
		glPopMatrix();
		//--------------------------------------------------------------------------------------------------------------------------------------
		//--------------------------------------------------------------------ПРАВАЯ РУКА--------------------------------------------------------
		glPushMatrix();
		{
			glTranslatef(0, 0, 30);
			glRotatef(180, 0, 1, 0);
			glTranslatef(-27, 0, 0);
			glRotatef(Ugol3, 1, 0, 0);
			{
				TextureStart("LADOWKI.bmp");
				glBegin(GL_POLYGON);
				glTexCoord2f(1, 1); glVertex3f(9, 6, 20);
				glTexCoord2f(0, 1); glVertex3f(9, 2, 20);
				glTexCoord2f(0, 0); glVertex3f(5, 2, 20);
				glTexCoord2f(1, 0); glVertex3f(5, 6, 20);
				glEnd();
				TextureEnd();

			}

			{
				TextureStart("PLECHIRYK.bmp");
				glBegin(GL_POLYGON);
				glTexCoord2f(0, 1); glVertex3f(9, 6, 10);
				glTexCoord2f(0, 0); glVertex3f(9, 2, 10);
				glTexCoord2f(1, 0); glVertex3f(5, 2, 10);
				glTexCoord2f(1, 1); glVertex3f(5, 6, 10);
				glEnd();
				TextureEnd();
			}

			{
				TextureStart("RYKI_BAZUKI.bmp");
				glBegin(GL_QUADS); //AB
				glTexCoord2f(1, 0); glVertex3f(9, 2, 20);
				glTexCoord2f(1, 1); glVertex3f(9, 2, 10);
				glTexCoord2f(0, 1); glVertex3f(9, 6, 10);
				glTexCoord2f(0, 0); glVertex3f(9, 6, 20);
				glEnd();
				TextureEnd();
			}

			{
				TextureStart("RYKI_BAZUKI.bmp");
				glBegin(GL_QUADS);//BC
				glTexCoord2f(1, 0); glVertex3f(9, 6, 20);
				glTexCoord2f(1, 1); glVertex3f(9, 6, 10);
				glTexCoord2f(0, 1); glVertex3f(5, 6, 10);
				glTexCoord2f(0, 0); glVertex3f(5, 6, 20);
				glEnd();
				TextureEnd();
			}

			{
				TextureStart("RYKI_BAZUKI.bmp");
				glBegin(GL_QUADS);//CD
				glTexCoord2f(0, 0); glVertex3f(5, 2, 20);
				glTexCoord2f(0, 1); glVertex3f(5, 2, 10);
				glTexCoord2f(1, 1); glVertex3f(5, 6, 10);
				glTexCoord2f(1, 0); glVertex3f(5, 6, 20);
				glEnd();
				TextureEnd();
			}

			{
				TextureStart("RYKI_BAZUKI.bmp");
				glBegin(GL_QUADS);//DA
				glTexCoord2f(1, 0); glVertex3f(5, 2, 20);
				glTexCoord2f(1, 1); glVertex3f(5, 2, 10);
				glTexCoord2f(0, 1); glVertex3f(9, 2, 10);
				glTexCoord2f(0, 0); glVertex3f(9, 2, 20);
				glEnd();
				TextureEnd();
			}
		}
		//SWORD
		{
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			glTranslatef(7, 1, 20);
			glRotatef(90, 0, 0, 1);
			glRotatef(63, 0, 1, 0);

			{
				TextureStart("SWORD.bmp");
				glBegin(GL_QUADS);
				glTexCoord2f(0, 0); glVertex3f(-2, 0, -2);
				glTexCoord2f(0, 0.09375); glVertex3f(-2, 0, 1);
				glTexCoord2f(0.09375, 0.09375); glVertex3f(1, 0, 1);
				glTexCoord2f(0.09375, 0); glVertex3f(1, 0, -2);
				glEnd();
				TextureEnd();
			}

			{
				TextureStart("SWORD2.bmp");
				glBegin(GL_QUADS);
				glTexCoord2f(0, 1); glVertex3f(1, 0, 0);
				glTexCoord2f(0, 0); glVertex3f(0, 0, 1);
				glTexCoord2f(1, 0); glVertex3f(4, 0, 5);
				glTexCoord2f(1, 1); glVertex3f(5, 0, 4);
				glEnd();
				TextureEnd();
			}
			{
				TextureStart("SWORD3.bmp");
				glBegin(GL_QUADS);
				glTexCoord2f(1, 0); glVertex3f(7, 0, 2);
				glTexCoord2f(1, 1); glVertex3f(2, 0, 7);
				glTexCoord2f(0, 1); glVertex3f(3, 0, 8);
				glTexCoord2f(0, 0); glVertex3f(8, 0, 3);
				glEnd();
				TextureEnd();
			}

			{
				TextureStart("SWORD4.bmp");
				glBegin(GL_QUADS);
				glTexCoord2f(1, 0); glVertex3f(6.5, 0, 4.5);
				glTexCoord2f(1, 1); glVertex3f(4.5, 0, 6.5);
				glTexCoord2f(0, 1); glVertex3f(14.5, 0, 16);
				glTexCoord2f(0, 0); glVertex3f(16, 0, 14.5);
				glEnd();
				TextureEnd();
			}

			{
				TextureStart("SWORD5.bmp");
				glBegin(GL_TRIANGLES);
				glTexCoord2f(1, 0); glVertex3f(14.5, 0, 16);
				glTexCoord2f(1, 1); glVertex3f(16, 0, 16.5);
				glTexCoord2f(0, 1); glVertex3f(16, 0, 14.5);
				glEnd();
				TextureEnd();
			}
			glDisable(GL_BLEND);
		}
		glPopMatrix();
	}
	glPopMatrix();

	//OCELOT
	glPushMatrix();
	{
		glTranslatef(0, -30, 28);
		///////////////////////TELO
		{
			TextureStart("SPINA.bmp");

			glBegin(GL_POLYGON);
			glNormal3f(0, 0, -1);
			glTexCoord2f(1, 1); glVertex3f(9, 14, 10);
			glTexCoord2f(0, 1); glVertex3f(9, 2, 10);
			glTexCoord2f(0, 0); glVertex3f(13, 2, 10);
			glTexCoord2f(1, 0); glVertex3f(13, 14, 10);
			glEnd();

			glBegin(GL_POLYGON);
			glNormal3f(0, 0, 1);
			glTexCoord2f(0, 1); glVertex3f(9, 14, 5);
			glTexCoord2f(0, 0); glVertex3f(9, 2, 5);
			glTexCoord2f(1, 0); glVertex3f(13, 2, 5);
			glTexCoord2f(1, 1); glVertex3f(13, 14, 5);
			glEnd();

			glBegin(GL_QUADS);
			glNormal3f(-2, 0, 1);
			glTexCoord2f(1, 0); glVertex3f(9, 2, 10);
			glTexCoord2f(1, 1); glVertex3f(9, 2, 5);
			glTexCoord2f(0, 1); glVertex3f(9, 14, 5);
			glTexCoord2f(0, 0); glVertex3f(9, 14, 10);
			glEnd();

			glBegin(GL_QUADS);
			glNormal3f(0, 5, 1);
			glTexCoord2f(0, 0); glVertex3f(9, 14, 10);
			glTexCoord2f(0, 1); glVertex3f(9, 14, 5);
			glTexCoord2f(1, 1); glVertex3f(13, 14, 5);
			glTexCoord2f(1, 0); glVertex3f(13, 14, 10);
			glEnd();

			glBegin(GL_QUADS);
			glNormal3f(2, 0, 1);
			glTexCoord2f(0, 0); glVertex3f(13, 2, 10);
			glTexCoord2f(0, 1); glVertex3f(13, 2, 5);
			glTexCoord2f(1, 1); glVertex3f(13, 14, 5);
			glTexCoord2f(1, 0); glVertex3f(13, 14, 10);
			glEnd();

			glBegin(GL_QUADS);
			glNormal3f(0, -5, 1);
			glTexCoord2f(1, 0); glVertex3f(13, 2, 10);
			glTexCoord2f(1, 1); glVertex3f(13, 2, 5);
			glTexCoord2f(0, 1); glVertex3f(9, 2, 5);
			glTexCoord2f(0, 0); glVertex3f(9, 2, 10);
			glEnd();

			TextureEnd();
		}
		/////////////////LAPA LEVAYA PEREDNYA
		{
			TextureStart("STOPA.bmp");

			glBegin(GL_POLYGON);
			glNormal3f(0, 0, -1);
			glTexCoord2f(1, 1); glVertex3f(11.5, 13, 10);
			glTexCoord2f(0, 1); glVertex3f(11.5, 11, 10);
			glTexCoord2f(0, 0); glVertex3f(13, 11, 10);
			glTexCoord2f(1, 0); glVertex3f(13, 13, 10);
			glEnd();

			glBegin(GL_POLYGON);
			glNormal3f(0, 0, 1);
			glTexCoord2f(0, 1); glVertex3f(11.5, 13, 2);
			glTexCoord2f(0, 0); glVertex3f(11.5, 11, 2);
			glTexCoord2f(1, 0); glVertex3f(13, 11, 2);
			glTexCoord2f(1, 1); glVertex3f(13, 13, 2);
			glEnd();

			glBegin(GL_QUADS);
			glNormal3f(-2, 0, 1);
			glTexCoord2f(1, 0); glVertex3f(11.5, 11, 10);
			glTexCoord2f(1, 1); glVertex3f(11.5, 11, 2);
			glTexCoord2f(0, 1); glVertex3f(11.5, 13, 2);
			glTexCoord2f(0, 0); glVertex3f(11.5, 13, 10);
			glEnd();

			glBegin(GL_QUADS);
			glNormal3f(0, 5, 1);
			glTexCoord2f(0, 0); glVertex3f(11.5, 13, 10);
			glTexCoord2f(0, 1); glVertex3f(11.5, 13, 2);
			glTexCoord2f(1, 1); glVertex3f(13, 13, 2);
			glTexCoord2f(1, 0); glVertex3f(13, 13, 10);
			glEnd();

			glBegin(GL_QUADS);
			glNormal3f(2, 0, 1);
			glTexCoord2f(0, 0); glVertex3f(13, 11, 10);
			glTexCoord2f(0, 1); glVertex3f(13, 11, 2);
			glTexCoord2f(1, 1); glVertex3f(13, 13, 2);
			glTexCoord2f(1, 0); glVertex3f(13, 13, 10);
			glEnd();

			glBegin(GL_QUADS);
			glNormal3f(0, -5, 1);
			glTexCoord2f(1, 0); glVertex3f(13, 11, 10);
			glTexCoord2f(1, 1); glVertex3f(13, 11, 2);
			glTexCoord2f(0, 1); glVertex3f(11.5, 11, 2);
			glTexCoord2f(0, 0); glVertex3f(11.5, 11, 10);
			glEnd();

			TextureEnd();
		}
		/////////////////LAPA PRAVAYA PEREDNYA
		{
			TextureStart("STOPA.bmp");

			glBegin(GL_POLYGON);
			glNormal3f(0, 0, -1);
			glTexCoord2f(1, 1); glVertex3f(9, 13, 10);
			glTexCoord2f(0, 1); glVertex3f(9, 11, 10);
			glTexCoord2f(0, 0); glVertex3f(10.5, 11, 10);
			glTexCoord2f(1, 0); glVertex3f(10.5, 13, 10);
			glEnd();

			glBegin(GL_POLYGON);
			glNormal3f(0, 0, 1);
			glTexCoord2f(0, 1); glVertex3f(9, 13, 2);
			glTexCoord2f(0, 0); glVertex3f(9, 11, 2);
			glTexCoord2f(1, 0); glVertex3f(10.5, 11, 2);
			glTexCoord2f(1, 1); glVertex3f(10.5, 13, 2);
			glEnd();

			glBegin(GL_QUADS);
			glNormal3f(-2, 0, 1);
			glTexCoord2f(1, 0); glVertex3f(9, 11, 10);
			glTexCoord2f(1, 1); glVertex3f(9, 11, 2);
			glTexCoord2f(0, 1); glVertex3f(9, 13, 2);
			glTexCoord2f(0, 0); glVertex3f(9, 13, 10);
			glEnd();

			glBegin(GL_QUADS);
			glNormal3f(0, 5, 1);
			glTexCoord2f(0, 0); glVertex3f(9, 13, 10);
			glTexCoord2f(0, 1); glVertex3f(9, 13, 2);
			glTexCoord2f(1, 1); glVertex3f(10.5, 13, 2);
			glTexCoord2f(1, 0); glVertex3f(10.5, 13, 10);
			glEnd();

			glBegin(GL_QUADS);
			glNormal3f(2, 0, 1);
			glTexCoord2f(0, 0); glVertex3f(10.5, 11, 10);
			glTexCoord2f(0, 1); glVertex3f(10.5, 11, 2);
			glTexCoord2f(1, 1); glVertex3f(10.5, 13, 2);
			glTexCoord2f(1, 0); glVertex3f(10.5, 13, 10);
			glEnd();

			glBegin(GL_QUADS);
			glNormal3f(0, -5, 1);
			glTexCoord2f(1, 0); glVertex3f(10.5, 11, 10);
			glTexCoord2f(1, 1); glVertex3f(10.5, 11, 2);
			glTexCoord2f(0, 1); glVertex3f(9, 11, 2);
			glTexCoord2f(0, 0); glVertex3f(9, 11, 10);
			glEnd();

			TextureEnd();
		}
		//////////////////////LEVAYA LAPA ZADNYYAAY
		{
			TextureStart("STOPA.bmp");

			glBegin(GL_POLYGON);
			glNormal3f(0, 0, -1);
			glTexCoord2f(1, 1); glVertex3f(11.5, 5, 10);
			glTexCoord2f(0, 1); glVertex3f(11.5, 3, 10);
			glTexCoord2f(0, 0); glVertex3f(13, 3, 10);
			glTexCoord2f(1, 0); glVertex3f(13, 5, 10);
			glEnd();

			glBegin(GL_POLYGON);
			glNormal3f(0, 0, 1);
			glTexCoord2f(0, 1); glVertex3f(11.5, 5, 2);
			glTexCoord2f(0, 0); glVertex3f(11.5, 3, 2);
			glTexCoord2f(1, 0); glVertex3f(13, 3, 2);
			glTexCoord2f(1, 1); glVertex3f(13, 5, 2);
			glEnd();

			glBegin(GL_QUADS);
			glNormal3f(-2, 0, 1);
			glTexCoord2f(1, 0); glVertex3f(11.5, 3, 10);
			glTexCoord2f(1, 1); glVertex3f(11.5, 3, 2);
			glTexCoord2f(0, 1); glVertex3f(11.5, 5, 2);
			glTexCoord2f(0, 0); glVertex3f(11.5, 5, 10);
			glEnd();

			glBegin(GL_QUADS);
			glNormal3f(0, 5, 1);
			glTexCoord2f(0, 0); glVertex3f(11.5, 5, 10);
			glTexCoord2f(0, 1); glVertex3f(11.5, 5, 2);
			glTexCoord2f(1, 1); glVertex3f(13, 5, 2);
			glTexCoord2f(1, 0); glVertex3f(13, 5, 10);
			glEnd();

			glBegin(GL_QUADS);
			glNormal3f(2, 0, 1);
			glTexCoord2f(0, 0); glVertex3f(13, 3, 10);
			glTexCoord2f(0, 1); glVertex3f(13, 3, 2);
			glTexCoord2f(1, 1); glVertex3f(13, 5, 2);
			glTexCoord2f(1, 0); glVertex3f(13, 5, 10);
			glEnd();

			glBegin(GL_QUADS);
			glNormal3f(0, -5, 1);
			glTexCoord2f(1, 0); glVertex3f(13, 3, 10);
			glTexCoord2f(1, 1); glVertex3f(13, 3, 2);
			glTexCoord2f(0, 1); glVertex3f(11.5, 3, 2);
			glTexCoord2f(0, 0); glVertex3f(11.5, 3, 10);
			glEnd();

			TextureEnd();
		}
		/////////////////LAPA PRAVAYA ZADNYAYA
		{
			TextureStart("STOPA.bmp");

			glBegin(GL_POLYGON);
			glNormal3f(0, 0, -1);
			glTexCoord2f(1, 1); glVertex3f(9, 5, 10);
			glTexCoord2f(0, 1); glVertex3f(9, 3, 10);
			glTexCoord2f(0, 0); glVertex3f(10.5, 3, 10);
			glTexCoord2f(1, 0); glVertex3f(10.5, 5, 10);
			glEnd();

			glBegin(GL_POLYGON);
			glNormal3f(0, 0, 1);
			glTexCoord2f(0, 1); glVertex3f(9, 5, 2);
			glTexCoord2f(0, 0); glVertex3f(9, 3, 2);
			glTexCoord2f(1, 0); glVertex3f(10.5, 3, 2);
			glTexCoord2f(1, 1); glVertex3f(10.5, 5, 2);
			glEnd();

			glBegin(GL_QUADS);
			glNormal3f(-2, 0, 1);
			glTexCoord2f(1, 0); glVertex3f(9, 3, 10);
			glTexCoord2f(1, 1); glVertex3f(9, 3, 2);
			glTexCoord2f(0, 1); glVertex3f(9, 5, 2);
			glTexCoord2f(0, 0); glVertex3f(9, 5, 10);
			glEnd();

			glBegin(GL_QUADS);
			glNormal3f(0, 5, 1);
			glTexCoord2f(0, 0); glVertex3f(9, 5, 10);
			glTexCoord2f(0, 1); glVertex3f(9, 5, 2);
			glTexCoord2f(1, 1); glVertex3f(10.5, 5, 2);
			glTexCoord2f(1, 0); glVertex3f(10.5, 5, 10);
			glEnd();

			glBegin(GL_QUADS);
			glNormal3f(2, 0, 1);
			glTexCoord2f(0, 0); glVertex3f(10.5, 3, 10);
			glTexCoord2f(0, 1); glVertex3f(10.5, 3, 2);
			glTexCoord2f(1, 1); glVertex3f(10.5, 5, 2);
			glTexCoord2f(1, 0); glVertex3f(10.5, 5, 10);
			glEnd();

			glBegin(GL_QUADS);
			glNormal3f(0, -5, 1);
			glTexCoord2f(1, 0); glVertex3f(10.5, 3, 10);
			glTexCoord2f(1, 1); glVertex3f(10.5, 3, 2);
			glTexCoord2f(0, 1); glVertex3f(9, 3, 2);
			glTexCoord2f(0, 0); glVertex3f(9, 3, 10);
			glEnd();

			TextureEnd();
			/////////////////HVOST
			{

				glPushMatrix();
				TextureStart("SPINA.bmp");

				glBegin(GL_POLYGON);
				glNormal3f(0, 0, -1);
				glTexCoord2f(1, 1); glVertex3f(10.7, 2, 9);
				glTexCoord2f(0, 1); glVertex3f(10.7, -5, 9);
				glTexCoord2f(0, 0); glVertex3f(11.3, -5, 9);
				glTexCoord2f(1, 0); glVertex3f(11.3, 2, 9);
				glEnd();

				glBegin(GL_POLYGON);
				glNormal3f(0, 0, 1);
				glTexCoord2f(0, 1); glVertex3f(10.7, 2, 8);
				glTexCoord2f(0, 0); glVertex3f(10.7, -5, 8);
				glTexCoord2f(1, 0); glVertex3f(11.3, -5, 8);
				glTexCoord2f(1, 1); glVertex3f(11.3, 2, 8);
				glEnd();

				glBegin(GL_QUADS);
				glNormal3f(-2, 0, 1);
				glTexCoord2f(1, 0); glVertex3f(10.7, -5, 9);
				glTexCoord2f(1, 1); glVertex3f(10.7, -5, 8);
				glTexCoord2f(0, 1); glVertex3f(10.7, 2, 8);
				glTexCoord2f(0, 0); glVertex3f(10.7, 2, 9);
				glEnd();

				glBegin(GL_QUADS);
				glNormal3f(0, 5, 1);
				glTexCoord2f(0, 0); glVertex3f(10.7, 2, 9);
				glTexCoord2f(0, 1); glVertex3f(10.7, 2, 8);
				glTexCoord2f(1, 1); glVertex3f(11.3, 2, 8);
				glTexCoord2f(1, 0); glVertex3f(11.3, 2, 9);
				glEnd();

				glBegin(GL_QUADS);
				glNormal3f(2, 0, 1);
				glTexCoord2f(0, 0); glVertex3f(11.3, -5, 9);
				glTexCoord2f(0, 1); glVertex3f(11.3, -5, 8);
				glTexCoord2f(1, 1); glVertex3f(11.3, 2, 8);
				glTexCoord2f(1, 0); glVertex3f(11.3, 2, 9);
				glEnd();

				glBegin(GL_QUADS);
				glNormal3f(0, -5, 1);
				glTexCoord2f(1, 0); glVertex3f(11.3, -5, 9);
				glTexCoord2f(1, 1); glVertex3f(11.3, -5, 8);
				glTexCoord2f(0, 1); glVertex3f(10.7, -5, 8);
				glTexCoord2f(0, 0); glVertex3f(10.7, -5, 9);
				glEnd();

				TextureEnd();
			glPopMatrix();
			}
			///////////////////////ROWA
			{
				TextureStart("SPINA.bmp");

				glBegin(GL_POLYGON);
				glNormal3f(0, 0, -1);
				glTexCoord2f(0, 1); glVertex3f(9, 17, 11.5);
				glTexCoord2f(0, 0); glVertex3f(9, 14, 11.5);
				glTexCoord2f(1, 0); glVertex3f(13, 14, 11.5);
				glTexCoord2f(1, 1); glVertex3f(13, 17, 11.5);
				glEnd();

				glBegin(GL_POLYGON);
				glNormal3f(0, 0, 1);
				glTexCoord2f(0, 1); glVertex3f(9, 17, 7.5);
				glTexCoord2f(0, 0); glVertex3f(9, 14, 7.5);
				glTexCoord2f(1, 0); glVertex3f(13, 14, 7.5);
				glTexCoord2f(1, 1); glVertex3f(13, 17, 7.5);
				glEnd();

				glBegin(GL_QUADS);
				glNormal3f(-2, 0, 1);
				glTexCoord2f(0, 1); glVertex3f(9, 14, 11.5);
				glTexCoord2f(0, 0); glVertex3f(9, 14, 7.5);
				glTexCoord2f(1, 0); glVertex3f(9, 17, 7.5);
				glTexCoord2f(1, 1); glVertex3f(9, 17, 11.5);
				glEnd();
				TextureEnd();

				TextureStart("POWA.bmp");
				glBegin(GL_QUADS);
				glNormal3f(0, 5, 1);
				glTexCoord2f(0, 1); glVertex3f(9, 17, 11.5);
				glTexCoord2f(0, 0); glVertex3f(9, 17, 7.5);
				glTexCoord2f(1, 0); glVertex3f(13, 17, 7.5);
				glTexCoord2f(1, 1); glVertex3f(13, 17, 11.5);
				glEnd();
				TextureEnd();

				TextureStart("SPINA.bmp");
				glBegin(GL_QUADS);
				glNormal3f(2, 0, 1);
				glTexCoord2f(0, 1); glVertex3f(13, 14, 11.5);
				glTexCoord2f(0, 0); glVertex3f(13, 14, 7.5);
				glTexCoord2f(1, 0); glVertex3f(13, 17, 7.5);
				glTexCoord2f(0, 1); glVertex3f(13, 17, 11.5);
				glEnd();

				glBegin(GL_QUADS);
				glNormal3f(0, -5, 1);
				glTexCoord2f(1, 1); glVertex3f(13, 14, 11.5);
				glTexCoord2f(1, 0); glVertex3f(13, 14, 7.5);
				glTexCoord2f(0, 0); glVertex3f(9, 14, 7.5);
				glTexCoord2f(0, 1); glVertex3f(9, 14, 11.5);
				glEnd();
				TextureEnd();
			}
			///////////////////////WNOBEL
			{
				TextureStart("STOPA.bmp");

				glBegin(GL_POLYGON);
				glNormal3f(0, 0, -1);
				glTexCoord2f(0, 1); glVertex3f(10, 18, 9.5);
				glTexCoord2f(0, 0); glVertex3f(10, 17, 9.5);
				glTexCoord2f(1, 0); glVertex3f(12, 17, 9.5);
				glTexCoord2f(1, 1); glVertex3f(12, 18, 9.5);
				glEnd();

				glBegin(GL_POLYGON);
				glNormal3f(0, 0, 1);
				glTexCoord2f(0, 1); glVertex3f(10, 18, 7.5);
				glTexCoord2f(0, 0); glVertex3f(10, 17, 7.5);
				glTexCoord2f(1, 0); glVertex3f(12, 17, 7.5);
				glTexCoord2f(1, 1); glVertex3f(12, 18, 7.5);
				glEnd();

				glBegin(GL_QUADS);
				glNormal3f(-2, 0, 1);
				glTexCoord2f(0, 1); glVertex3f(10, 17, 9.5);
				glTexCoord2f(0, 0); glVertex3f(10, 17, 7.5);
				glTexCoord2f(1, 0); glVertex3f(10, 18, 7.5);
				glTexCoord2f(1, 1); glVertex3f(10, 18, 9.5);
				glEnd();
				TextureEnd();

				TextureStart("NOS.bmp");
				glBegin(GL_QUADS);
				glNormal3f(0, 5, 1);
				glTexCoord2f(0, 1); glVertex3f(10, 18, 9.5);
				glTexCoord2f(0, 0); glVertex3f(10, 18, 7.5);
				glTexCoord2f(1, 0); glVertex3f(12, 18, 7.5);
				glTexCoord2f(1, 1); glVertex3f(12, 18, 9.5);
				glEnd();
				TextureEnd();

				TextureStart("STOPA.bmp");
				glBegin(GL_QUADS);
				glNormal3f(2, 0, 1);
				glTexCoord2f(0, 1); glVertex3f(12, 17, 9.5);
				glTexCoord2f(0, 0); glVertex3f(12, 17, 7.5);
				glTexCoord2f(1, 0); glVertex3f(12, 18, 7.5);
				glTexCoord2f(0, 1); glVertex3f(12, 18, 9.5);
				glEnd();

				glBegin(GL_QUADS);
				glNormal3f(0, -5, 1);
				glTexCoord2f(1, 1); glVertex3f(12, 17, 9.5);
				glTexCoord2f(1, 0); glVertex3f(12, 17, 7.5);
				glTexCoord2f(0, 0); glVertex3f(10, 17, 7.5);
				glTexCoord2f(0, 1); glVertex3f(10, 17, 9.5);
				glEnd();

				TextureEnd();
			}
			///////////////////////LEVOE YHO
			{
				TextureStart("STOPA.bmp");

				glBegin(GL_POLYGON);
				glNormal3f(0, 0, -1);
				glTexCoord2f(1, 1); glVertex3f(12, 15, 12.5);
				glTexCoord2f(0, 1); glVertex3f(12, 14, 12.5);
				glTexCoord2f(0, 0); glVertex3f(12.5, 14, 12.5);
				glTexCoord2f(1, 0); glVertex3f(12.5, 15, 12.5);
				glEnd();

				glBegin(GL_POLYGON);
				glNormal3f(0, 0, 1);
				glTexCoord2f(0, 1); glVertex3f(12, 15, 11.5);
				glTexCoord2f(0, 0); glVertex3f(12, 14, 11.5);
				glTexCoord2f(1, 0); glVertex3f(12.5, 14, 11.5);
				glTexCoord2f(1, 1); glVertex3f(12.5, 15, 11.5);
				glEnd();

				glBegin(GL_QUADS);
				glNormal3f(-2, 0, 1);
				glTexCoord2f(1, 0); glVertex3f(12, 14, 12.5);
				glTexCoord2f(1, 1); glVertex3f(12, 14, 11.5);
				glTexCoord2f(0, 1); glVertex3f(12, 15, 11.5);
				glTexCoord2f(0, 0); glVertex3f(12, 15, 12.5);
				glEnd();

				glBegin(GL_QUADS);
				glNormal3f(0, 5, 1);
				glTexCoord2f(0, 0); glVertex3f(12, 15, 12.5);
				glTexCoord2f(0, 1); glVertex3f(12, 15, 11.5);
				glTexCoord2f(1, 1); glVertex3f(12.5, 15, 11.5);
				glTexCoord2f(1, 0); glVertex3f(12.5, 15, 12.5);
				glEnd();

				glBegin(GL_QUADS);
				glNormal3f(2, 0, 1);
				glTexCoord2f(0, 0); glVertex3f(12.5, 14, 12.5);
				glTexCoord2f(0, 1); glVertex3f(12.5, 14, 11.5);
				glTexCoord2f(1, 1); glVertex3f(12.5, 15, 11.5);
				glTexCoord2f(1, 0); glVertex3f(12.5, 15, 12.5);
				glEnd();

				glBegin(GL_QUADS);
				glNormal3f(0, -5, 1);
				glTexCoord2f(1, 0); glVertex3f(12.5, 14, 12.5);
				glTexCoord2f(1, 1); glVertex3f(12.5, 14, 11.5);
				glTexCoord2f(0, 1); glVertex3f(12, 14, 11.5);
				glTexCoord2f(0, 0); glVertex3f(12, 14, 12.5);
				glEnd();

				TextureEnd();
			}
			///////////////////////PRAVOE YHO
			{
				TextureStart("STOPA.bmp");

				glBegin(GL_POLYGON);
				glNormal3f(0, 0, -1);
				glTexCoord2f(1, 1); glVertex3f(9.5, 15, 12.5);
				glTexCoord2f(0, 1); glVertex3f(9.5, 14, 12.5);
				glTexCoord2f(0, 0); glVertex3f(10, 14, 12.5);
				glTexCoord2f(1, 0); glVertex3f(10, 15, 12.5);
				glEnd();

				glBegin(GL_POLYGON);
				glNormal3f(0, 0, 1);
				glTexCoord2f(0, 1); glVertex3f(9.5, 15, 11.5);
				glTexCoord2f(0, 0); glVertex3f(9.5, 14, 11.5);
				glTexCoord2f(1, 0); glVertex3f(10, 14, 11.5);
				glTexCoord2f(1, 1); glVertex3f(10, 15, 11.5);
				glEnd();

				glBegin(GL_QUADS);
				glNormal3f(-2, 0, 1);
				glTexCoord2f(1, 0); glVertex3f(9.5, 14, 12.5);
				glTexCoord2f(1, 1); glVertex3f(9.5, 14, 11.5);
				glTexCoord2f(0, 1); glVertex3f(9.5, 15, 11.5);
				glTexCoord2f(0, 0); glVertex3f(9.5, 15, 12.5);
				glEnd();

				glBegin(GL_QUADS);
				glNormal3f(0, 5, 1);
				glTexCoord2f(0, 0); glVertex3f(9.5, 15, 12.5);
				glTexCoord2f(0, 1); glVertex3f(9.5, 15, 11.5);
				glTexCoord2f(1, 1); glVertex3f(10, 15, 11.5);
				glTexCoord2f(1, 0); glVertex3f(10, 15, 12.5);
				glEnd();

				glBegin(GL_QUADS);
				glNormal3f(2, 0, 1);
				glTexCoord2f(0, 0); glVertex3f(10, 14, 12.5);
				glTexCoord2f(0, 1); glVertex3f(10, 14, 11.5);
				glTexCoord2f(1, 1); glVertex3f(10, 15, 11.5);
				glTexCoord2f(1, 0); glVertex3f(10, 15, 12.5);
				glEnd();

				glBegin(GL_QUADS);
				glNormal3f(0, -5, 1);
				glTexCoord2f(1, 0); glVertex3f(10, 14, 12.5);
				glTexCoord2f(1, 1); glVertex3f(10, 14, 11.5);
				glTexCoord2f(0, 1); glVertex3f(9.5, 14, 11.5);
				glTexCoord2f(0, 0); glVertex3f(9.5, 14, 12.5);
				glEnd();

				TextureEnd();
			}
		}
	}
	glPopMatrix();

	//TRAVYSHKA
	/*{
		glRotatef(TRAVYSHKA, 1, 0, 0);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		for (int a= 0; a <= 30; a+= 10)
		{
			for (int b =0; b <= 35; b+= 10)
			{
				TextureStart("TRAVYSHKA.bmp");
				glBegin(GL_QUADS);
				glTexCoord2f(0, 0); glVertex3f(a, b, 0);
				glTexCoord2f(0, 1); glVertex3f(a, b, 20);
				glTexCoord2f(1, 1); glVertex3f(a+5, b, 20);
				glTexCoord2f(1, 0); glVertex3f(a+5, b, 0);
				glEnd();
				TextureEnd();
			}
		}
		glDisable(GL_BLEND);
	}*/
	glPushMatrix();
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	{
		glTranslatef(4, 20, 0);
		TextureStart("DONBASS.bmp");
		glBegin(GL_QUADS);
		glTexCoord2f(0, 1); glVertex3f(0, 20, 1);
		glTexCoord2f(0, 0); glVertex3f(0, 0, 1);
		glTexCoord2f(1, 0); glVertex3f(20, 0, 1);
		glTexCoord2f(1, 1); glVertex3f(20, 20, 1);
		glEnd();
		TextureEnd();
	}

	glTranslatef(0, -35, 0);

	{
		TextureStart("WOOD.bmp");

		glBegin(GL_QUADS);
		glTexCoord2f(0, 1); glVertex3f(25, 0, 0);
		glTexCoord2f(0, 0); glVertex3f(25, 0, 10);
		glTexCoord2f(3, 0); glVertex3f(75, 0, 10);
		glTexCoord2f(3, 1); glVertex3f(75, 0, 0);
		glEnd();

		glBegin(GL_QUADS);
		glTexCoord2f(0, 1); glVertex3f(25, 0, 20);
		glTexCoord2f(0, 0); glVertex3f(25, 0, 30);
		glTexCoord2f(3, 0); glVertex3f(75, 0, 30);
		glTexCoord2f(3, 1); glVertex3f(75, 0, 20);
		glEnd();

		glBegin(GL_QUADS);
		glTexCoord2f(0, 1); glVertex3f(0, 0, 0);
		glTexCoord2f(0, 0); glVertex3f(0, 0, 10);
		glTexCoord2f(3, 0); glVertex3f(-50, 0, 10);
		glTexCoord2f(3, 1); glVertex3f(-50, 0, 0);
		glEnd();

		glBegin(GL_QUADS);
		glTexCoord2f(0, 1); glVertex3f(0, 0, 20);
		glTexCoord2f(0, 0); glVertex3f(0, 0, 30);
		glTexCoord2f(3, 0); glVertex3f(-50, 0, 30);
		glTexCoord2f(3, 1); glVertex3f(-50, 0, 20);
		glEnd();

		glBegin(GL_QUADS);
		glTexCoord2f(0, 3); glVertex3f(-50, -100, 0);
		glTexCoord2f(0, 0); glVertex3f(-50, -100, 30);
		glTexCoord2f(8, 0); glVertex3f(75, -100, 30);
		glTexCoord2f(8, 3); glVertex3f(75, -100, 0);
		glEnd();

		TextureEnd();

		glPushMatrix();
		{
			TextureStart("WOOD.bmp");

			glBegin(GL_QUADS);
			glTexCoord2f(0, 1); glVertex3f(-50, 0, 0);
			glTexCoord2f(0, 0); glVertex3f(-50, 0, 10);
			glTexCoord2f(3, 0); glVertex3f(-50, -100, 10);
			glTexCoord2f(3, 1); glVertex3f(-50, -100, 0);
			glEnd();

			glBegin(GL_QUADS);
			glTexCoord2f(0, 1); glVertex3f(75, 0, 0);
			glTexCoord2f(0, 0); glVertex3f(75, 0, 10);
			glTexCoord2f(3, 0); glVertex3f(75, -100, 10);
			glTexCoord2f(3, 1); glVertex3f(75, -100, 0);
			glEnd();

			glBegin(GL_QUADS);
			glTexCoord2f(0, 1); glVertex3f(-50, 0, 20);
			glTexCoord2f(0, 0); glVertex3f(-50, 0, 30);
			glTexCoord2f(3, 0); glVertex3f(-50, -100, 30);
			glTexCoord2f(3, 1); glVertex3f(-50, -100, 20);
			glEnd();

			glBegin(GL_QUADS);
			glTexCoord2f(0, 1); glVertex3f(75, 0, 20);
			glTexCoord2f(0, 0); glVertex3f(75, 0, 30);
			glTexCoord2f(3, 0); glVertex3f(75, -100, 30);
			glTexCoord2f(3, 1); glVertex3f(75, -100, 20);
			glEnd();

			TextureEnd();
		}


		glPopMatrix();
	}

	{
		TextureStart("WOOD.bmp");
		glBegin(GL_QUADS);
		glTexCoord2f(0, 8); glVertex3f(75, -100, 30);
		glTexCoord2f(0, 0); glVertex3f(75, 0, 30);
		glTexCoord2f(8, 0); glVertex3f(-50, 0, 30);
		glTexCoord2f(8, 8); glVertex3f(-50, -100, 30);
		glEnd();
		TextureEnd();
	}

	{
		TextureStart("ALMAZ.bmp");
		glBegin(GL_QUADS);
		glTexCoord2f(0, 8); glVertex3f(75, 0, 0.5);
		glTexCoord2f(0, 0); glVertex3f(75, -100, 0.5);
		glTexCoord2f(8, 0); glVertex3f(-50, -100, 0.5);
		glTexCoord2f(8, 8); glVertex3f(-50, 0, 0.5);
		glEnd();
		TextureEnd();
	}

	glPushMatrix();
	{
		glRotatef(DOORS, 0, 0, 1);
		TextureStart("DOORS.bmp");
		glBegin(GL_QUADS);
		glTexCoord2f(0, 1); glVertex3f(0, 0, 0);
		glTexCoord2f(0, 0); glVertex3f(0, 0, 30);
		glTexCoord2f(1, 0); glVertex3f(25, 0, 30);
		glTexCoord2f(1, 1); glVertex3f(25, 0, 0);
		glEnd();
		TextureEnd();
	}
	glPopMatrix();

	{
		TextureStart("GLASS.bmp");



		glBegin(GL_QUADS);
		glTexCoord2f(0, 1); glVertex3f(-50, 0, 10);
		glTexCoord2f(0, 0); glVertex3f(-50, 0, 20);
		glTexCoord2f(3, 0); glVertex3f(-50, -100, 20);
		glTexCoord2f(3, 1); glVertex3f(-50, -100, 10);
		glEnd();

		glBegin(GL_QUADS);
		glTexCoord2f(0, 1); glVertex3f(75, 0, 10);
		glTexCoord2f(0, 0); glVertex3f(75, 0, 20);
		glTexCoord2f(3, 0); glVertex3f(75, -100, 20);
		glTexCoord2f(3, 1); glVertex3f(75, -100, 10);
		glEnd();

		glBegin(GL_QUADS);
		glTexCoord2f(0, 1); glVertex3f(0, 0, 10);
		glTexCoord2f(0, 0); glVertex3f(0, 0, 20);
		glTexCoord2f(3, 0); glVertex3f(-50, 0, 20);
		glTexCoord2f(3, 1); glVertex3f(-50, 0, 10);
		glEnd();

		glBegin(GL_QUADS);
		glTexCoord2f(0, 1); glVertex3f(25, 0, 10);
		glTexCoord2f(0, 0); glVertex3f(25, 0, 20);
		glTexCoord2f(3, 0); glVertex3f(75, 0, 20);
		glTexCoord2f(3, 1); glVertex3f(75, 0, 10);
		glEnd();

		TextureEnd();
	}
	glPopMatrix();


	//ZABOR
	{
		TextureStart("ZABOR.bmp");
		glTranslatef(-40, 140, 0);
		glBegin(GL_QUADS);
		glTexCoord2f(0, 1); glVertex3f(0, 0, 0);
		glTexCoord2f(0, 0); glVertex3f(0, 0, 20);
		glTexCoord2f(5, 0); glVertex3f(0, -120, 20);
		glTexCoord2f(5, 1); glVertex3f(0, -120, 0);
		glEnd();

		glBegin(GL_QUADS);
		glTexCoord2f(0, 1); glVertex3f(0, -120, 0);
		glTexCoord2f(0, 0); glVertex3f(0, -120, 20);
		glTexCoord2f(5, 0); glVertex3f(-100, -120, 20);
		glTexCoord2f(5, 1); glVertex3f(-100, -120, 0);
		glEnd();

		TextureEnd();
	}
	glDisable(GL_BLEND);
	glPopMatrix();
}
