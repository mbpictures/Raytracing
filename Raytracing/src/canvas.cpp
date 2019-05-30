#include "../inc/canvas.h"
#include <nanogui/theme.h>

namespace {
	std::vector<std::string> tokenize(const std::string &string,
		const std::string &delim = "\n",
		bool includeEmpty = false) {
		std::string::size_type lastPos = 0, pos = string.find_first_of(delim, lastPos);
		std::vector<std::string> tokens;

		while (lastPos != std::string::npos) {
			std::string substr = string.substr(lastPos, pos - lastPos);
			if (!substr.empty() || includeEmpty)
				tokens.push_back(std::move(substr));
			lastPos = pos;
			if (lastPos != std::string::npos) {
				lastPos += 1;
				pos = string.find_first_of(delim, lastPos);
			}
		}

		return tokens;
	}

	constexpr char const *const defaultImageViewVertexShader =
		R"(#version 330
        uniform vec2 scaleFactor;
        uniform vec2 position;
        in vec2 vertex;
        out vec2 uv;
        void main() {
            uv = vertex;
            vec2 scaledVertex = (vertex * scaleFactor) + position;
            gl_Position  = vec4(2.0*scaledVertex.x - 1.0,
                                1.0 - 2.0*scaledVertex.y,
                                0.0, 1.0);

        })";

	constexpr char const *const defaultImageViewFragmentShader =
		R"(#version 330
        uniform sampler2D image;
        out vec4 color;
        in vec2 uv;
        void main() {
            color = texture(image, uv);
        })";

}

NAMESPACE_BEGIN(nanogui)
Canvas2D::Canvas2D(Widget *parent, int x, int y)
	: ImageView(parent, 0) {
	setSize({ x, y });
	imageSizeX = x;
	imageSizeY = y;
	screenData = new u8**[y];
	for (int i = 0; i < y; i++) {
		screenData[i] = new u8*[x];
		for (int j = 0; j < x; j++) {
			screenData[i][j] = new u8[3];
		}
	}

	clearDisplay();
	glGenTextures(1, &mImageID);
	glBindTexture(GL_TEXTURE_2D, mImageID);
	
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, x, y, 0, GL_RGB, GL_UNSIGNED_BYTE, (GLvoid*)screenData);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	// Enable textures
	glEnable(GL_TEXTURE_2D);
	updateImageParameters();
	updateImage();
	/*mShader.init("ImageViewShader", defaultImageViewVertexShader, defaultImageViewFragmentShader);

	MatrixXu indices(3, 2);
	indices.col(0) << 0, 1, 2;
	indices.col(1) << 2, 3, 1;

	MatrixXf vertices(2, 4);
	vertices.col(0) << 0, 0;
	vertices.col(1) << 1, 0;
	vertices.col(2) << 0, 1;
	vertices.col(3) << 1, 1;

	mShader.bind();
	mShader.uploadIndices(indices);
	mShader.uploadAttrib("vertex", vertices);*/
}

void Canvas2D::drawPixel(double r, double g, double b, int x, int y) {
	if (x < 0 || x > imageSizeX || y < 0 || y > imageSizeY) return;
	screenData[y][x][0] = (u8) (r * 255);
	screenData[y][x][1] = (u8) (g * 255);
	screenData[y][x][2] = (u8) (b * 255);
	updateImage();
	return;
}
void Canvas2D::clearDisplay() {

	for (int i = 0; i < imageSizeY; i++) {
		for (int j = 0; j < imageSizeX; j++) {
			screenData[i][j][0] = screenData[i][j][1] = screenData[i][j][2] = 0;
		}
	}
}

void Canvas2D::updateImage() {
	glBindTexture(GL_TEXTURE_2D, mImageID);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, imageSizeX, imageSizeY, GL_RGB, GL_UNSIGNED_BYTE, (GLvoid*)screenData);
	return;
}

NAMESPACE_END(nanogui)