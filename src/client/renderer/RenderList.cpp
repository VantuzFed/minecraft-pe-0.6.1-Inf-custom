#include "RenderList.h"

#include "gles.h"
#include "RenderChunk.h"
#include "Tesselator.h"


#include <cstring>


RenderList::RenderList()
	:	inited(false),
	rendered(false),
	listIndex(0),
	bufferLimit(0),
	capacity(1024 * 4)
{
	lists = new int[capacity];
	rlists = new RenderChunk[capacity];

	for (int i = 0; i < capacity; ++i)
		rlists[i].vboId = -1;
}

RenderList::~RenderList() {
	delete[] lists;
	delete[] rlists;
}

void RenderList::ensureCapacity(int needed) {
	if (needed > capacity) {
		int newCap = capacity * 2;
		if (newCap < needed) newCap = needed;
		int* newLists = new int[newCap];
		RenderChunk* newRlists = new RenderChunk[newCap];
		if (listIndex > 0) {
			memcpy(newLists, lists, listIndex * sizeof(int));
			for (int i = 0; i < listIndex; ++i) {
				newRlists[i] = rlists[i];
			}
		}
		for (int i = listIndex; i < newCap; ++i) {
			newRlists[i].vboId = -1;
		}
		delete[] lists;
		delete[] rlists;
		lists = newLists;
		rlists = newRlists;
		capacity = newCap;
	}
}

void RenderList::init(double xOff, double yOff, double zOff) {
	inited = true;
	listIndex = 0;

	this->xOff = xOff;
	this->yOff = yOff;
	this->zOff = zOff;
}

void RenderList::add(int list) {
	ensureCapacity(listIndex + 1);
	lists[listIndex] = list;
}

void RenderList::addR(const RenderChunk& chunk) {
	ensureCapacity(listIndex + 1);
	rlists[listIndex] = chunk;
}

void RenderList::next() {
	ensureCapacity(listIndex + 1);
	++listIndex;
}

void RenderList::render() {

	if (!inited) return;
	if (!rendered) {
		bufferLimit = listIndex;
		listIndex = 0;
		rendered = true;
	}
	if (listIndex < bufferLimit) {
		glPushMatrix2();

		#ifndef USE_VBO
			glTranslated2(-xOff, -yOff, -zOff);
			glCallLists(bufferLimit, GL_UNSIGNED_INT, lists);
		#else
			renderChunks();
		#endif/*!USE_VBO*/

		glPopMatrix2();
	}
}

void RenderList::renderChunks() {
	glEnableClientState2(GL_VERTEX_ARRAY);
	glEnableClientState2(GL_COLOR_ARRAY);
	glEnableClientState2(GL_TEXTURE_COORD_ARRAY);
	glEnableClientState2(GL_NORMAL_ARRAY);

	const int Stride = VertexSizeBytes;

	for (int i = 0; i < bufferLimit; ++i) {
		RenderChunk& rc = rlists[i];

		double rx = rc.pos.x - xOff;
		double ry = rc.pos.y - yOff;
		double rz = rc.pos.z - zOff;
		glPushMatrix2();
		glTranslated2(rx, ry, rz);
		glBindBuffer2(GL_ARRAY_BUFFER, rc.vboId);

		glVertexPointer2	(3, GL_FLOAT, Stride,  0);
		glTexCoordPointer2	(2, GL_FLOAT, Stride, (GLvoid*) (3 * 4));
		glColorPointer2		(4, GL_UNSIGNED_BYTE, Stride, (GLvoid*) (5 * 4));
		glNormalPointer		(GL_FLOAT, Stride, (GLvoid*) (6 * 4));

		glDrawArrays2(GL_TRIANGLES, 0, rc.vertexCount);

		glPopMatrix2();
	}

	glDisableClientState2(GL_NORMAL_ARRAY);
	glDisableClientState2(GL_VERTEX_ARRAY);
	glDisableClientState2(GL_COLOR_ARRAY);
	glDisableClientState2(GL_TEXTURE_COORD_ARRAY);
}

void RenderList::clear() {
	inited = false;
	rendered = false;
	listIndex = 0;
	bufferLimit = 0;
}
