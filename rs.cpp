// CRT
#include <stdio.h>
#include <string.h>

#include <iostream>
using namespace std;

// QT

#include <QtCore/QHash>
#include <QtCore/QDateTime>

#define BUFFERSIZE 16384
//#define BUFFERSIZE 65536
//#define BUFFERSIZE 1048576
//
struct Storage
{
	float m_bb[6];
	float* m_pt;
	char* m_fn;
	unsigned int m_sz;

	void Spool()
	{
		FILE* f = fopen(m_fn,"ab");
		fwrite((const char*)m_pt,32,m_sz,f);
		fclose(f);
		m_sz = 0;
	};

	bool outside(float*f)
	{
		if(f[0]<m_bb[0]) return true;
		if(f[0]>m_bb[1]) return true;
		if(f[1]<m_bb[2]) return true;
		if(f[1]>m_bb[3]) return true;
		if(f[2]<m_bb[4]) return true;
		if(f[2]>m_bb[5]) return true;
		return false;
	}

public:
	Storage() : m_sz(0) { m_pt = new float[8*BUFFERSIZE]; };
	void Setup(const char* c, float* bb)
	{
		memcpy(m_bb,bb,24);
		m_fn = strdup(c);
		FILE* f = fopen(m_fn,"w"); fclose(f);
	};
	void insert(float* f)
	{
		unsigned int off = m_sz;
		memcpy(m_pt+(off<<3),f,32);

		m_sz++;
		if(m_sz == BUFFERSIZE) Spool();
	};
	bool test(float* f)
	{
		if(outside(f)) return false;
		insert(f);
		return true;
	};
	~Storage()
	{
		if(m_sz == 0) return;
		Spool();
		delete [] m_pt;
	}
};

#define CHUNCKSIZE 4*BUFFERSIZE
float buf[8*CHUNCKSIZE];

struct triple 
{
	int x,y,z;
	triple(int X, int Y, int Z) : x(X) , y(Y) , z(Z) {};
	triple(const int* C) : x(C[0]) , y(C[1]) , z(C[2]) {};
	//bool operator==(const triple s) { return (x==s.x)&&(y==s.y)&&(z=s.z); };
	friend bool operator==(const triple &t1, const triple &t2);
	operator quint64() { return quint64((((x<<12)+y)<<12)+z); };
};

inline bool operator==(const triple &t1, const triple &t2)
{
	return (t1.x==t2.x)&&(t1.y==t2.y)&&(t1.z==t2.z);
};

inline uint qHash(triple key) { return qHash(quint64(key)); };

int main(int argc, char *argv[])
{
	{
		QString now = (QDateTime::currentDateTime()).toString("hh:mm:ss.zzz");
		QByteArray ba = now.toAscii();
		cout << ba.constData() << endl;
	}
	const char* ptc = argv[1];
	const char* map = argv[2];
	const char* clause = argv[3];

	// GET A MAP
	
	cout << "Load map..." << endl;

	FILE* fm = fopen(map,"rb");

	// COUNT
	int cnt = 0;
	fread(&cnt,sizeof(int),1,fm);

	// MAIN BBOX
	float B[6];
	fread(B,sizeof(float),6,fm);

	// MAIN METRICS
	int m[3];
	fread(m,sizeof(int),3,fm);
	cout << "...done" << endl;

	// STORAGES
	cout << "Build storages..." << endl;
	Storage* sc = new Storage[cnt];

	QHash<triple,Storage*> hm;

	for(int i=0;i<cnt;i++)
	{
		// BBOX
		float b[6];
		fread(b,sizeof(float),6,fm);

		// CAT
		int cat[3];
		fread(cat,sizeof(int),3,fm);

		// FILE
		char nmbuff[1024];
		sprintf(nmbuff,"%s.%04i.ptc",clause,i);

		//  SETUP
		sc[i].Setup(nmbuff,b);

		// HASHED
		triple t(cat);
		hm.insert(t,sc+i);
	};

	fclose(fm);
	cout << "...done" << endl;

	// PROCESS

	cout << "Process points..." << endl;

	FILE* f = fopen(ptc,"rb");

	int i;
	float* off;
	size_t r;

	int total = 0;

	float wd[3] = { B[1]-B[0], B[3]-B[2], B[5]-B[4] };

	int fails = 0;

	int cc = 0;

	do 
	{
		r = fread((char*)buf,32,CHUNCKSIZE,f);
		off = buf;
		for(i=0;i<r;i++)
		{
			triple T( 
				(off[0]-B[0])/wd[0]*m[0],
				(off[1]-B[2])/wd[1]*m[1],
				(off[2]-B[4])/wd[2]*m[2] );

			if(hm.contains(T)) hm[T]->insert(off);
			else
			{
				fails++;
				//for(int j=0;j<cnt;j++) if(sc[j].test(off)) { fails--; break; }
			}
			off+=8;
		}
		total += r;
		cc++;
		if(cc%100 == 0) cout << total << endl;
	} 
	while(!feof(f));

	fclose(f);

	cout << "...done" << endl;

	cout << "Freeing map..." << endl;
	delete [] sc;
	cout << "...done" << endl;

	cout << "Failed seeds: " << fails << endl;

	{
		QString now = QDateTime::currentDateTime().toString("hh:mm:ss.zzz");
		QByteArray ba = now.toAscii();
		cout << ba.constData() << endl;
	}

	//cout << "Press any key..." << endl;
	//char c;
	//cin >> c;
}
