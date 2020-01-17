#include "ofxCarousel.h"

#include "of3dGraphics.h"
#include "ofGraphics.h"

float wrap( float v, float l )
{
	if ( v < 0 ) return l - fmodf( -v, l );
	if ( v > l ) return fmodf( v, l );
	return v;
}

float easeOut( float k )
{
	k--;
	return k * k * k + 1;
}

ofxCarousel::ofxCarousel()
{
	mode = ofxCarouselModeWrap;

	length   = 360.0;
	position = 0.0;
	velocity = 0.0;
	friction = 0.96;
	bounce   = 0.86;
	elastic  = 0.5;

	updateState = UpdateMode::NONE;  // 0;
}

void ofxCarousel::grab( float reference )
{
	moveOffset    = 0;
	grabReference = reference;
	updateState   = UpdateMode::MOVE;  // 1;
}

void ofxCarousel::drag( float reference )
{
	moveOffset += reference - grabReference;
	grabReference = reference;
	updateState   = UpdateMode::MOVE;  // 1;
}

void ofxCarousel::move( float offset, float duration )
{
	if ( duration > 0.0 ) {
		animateTime     = ofGetElapsedTimef();
		animateDuration = duration;
		animateBegin    = position;
		animateDelta    = offset;
		updateState     = UpdateMode::ANIMATE;  // 3;
	} else {
		moveOffset  = offset;
		updateState = UpdateMode::MOVE;  // 1
	}
}

void ofxCarousel::drop()
{
	if ( pagingSize > 0 ) {
		float weight  = ( velocity == 0 ? 0 : velocity / fabsf( velocity ) * 0.51 );
		float nearest = roundf( position / pagingSize + weight ) * pagingSize;
		if ( mode == ofxCarouselModeStop ) {
			nearest = std::min( length - pagingSize, std::max( nearest, 0.f ) );
		}
		if ( fabsf( nearest - position ) > 0.01 ) {
			this->move( nearest - position, 0.4 );
		}
	} else {
		updateState = UpdateMode::DECELERATE;  // 2;
	}
}

void ofxCarousel::drop( float velocity )
{
	this->velocity = velocity;
	this->drop();
}

void ofxCarousel::update()
{
	auto pState = updateState;
	switch ( updateState ) {
		case UpdateMode::MOVE:
			this->updateMove();
			break;
		case UpdateMode::DECELERATE:
			this->updateDecelerate();
			break;
		case UpdateMode::ANIMATE:
			this->updateAnimate();
			break;
	}
	if ( pagingSize != 0 && pState == UpdateMode::ANIMATE && updateState == UpdateMode::NONE ) {
		float page = position / pagingSize;
		pagedEvent.notify( page );
	}
}

void ofxCarousel::updateMove()
{
	velocity = moveOffset;
	position += velocity;
	moveOffset = 0;

	if ( mode == ofxCarouselModeStop ) {
		if ( position < 0 ) {
			stretched += velocity * elastic;
			position = 0 + stretched;
			velocity = 0;
		} else if ( position > length ) {
			stretched += velocity * elastic;
			position = length + stretched;
			velocity = 0;
		} else {
			stretched = 0;
		}
	} else {
		position = wrap( position, length );
	}

	if ( fabsf( velocity ) < 0.01 ) {
		velocity    = 0;
		updateState = UpdateMode::NONE;  // 0;
	}
}

void ofxCarousel::updateDecelerate()
{
	velocity *= friction;
	position += velocity;

	if ( mode == ofxCarouselModeStop ) {
		if ( position <= 0 ) {
			stretched = position - 0;
			velocity  = fminf( velocity, 0 ) - stretched * ( 1 - bounce );
		} else if ( position >= length ) {
			stretched = position - length;
			velocity  = fmaxf( velocity, 0 ) - stretched * ( 1 - bounce );
		} else {
			stretched = 0;
		}
	} else {
		position = wrap( position, length );
	}

	if ( fabsf( velocity ) < 0.01 ) {
		velocity    = 0;
		updateState = UpdateMode::NONE;  // 0;
		position    = fmaxf( position, 0 );
		position    = fminf( position, length );
	}
}

void ofxCarousel::updateAnimate()
{
	float time = ( ofGetElapsedTimef() - animateTime ) / animateDuration;

	if ( time < 1 ) {
		position = animateBegin + animateDelta * easeOut( time );
	} else {
		position    = animateBegin + animateDelta;
		updateState = UpdateMode::NONE;  // 0;
	}
	if ( mode == ofxCarouselModeWrap ) {
		position = wrap( position, length );
	}
}

float ofxCarousel::positionForTile( int tile, int numberOfTiles, float tileSize, int* index )
{
	float pos = position + ( numberOfTiles - 1 - tile ) * tileSize;

	if ( index ) {
		*index = ( int )floorf( pos / ( tileSize * numberOfTiles ) ) * numberOfTiles + tile;
	}

	if ( pos < 0 ) {
		return 0 - tileSize - pos;
	}

	float modSize = tileSize * numberOfTiles;

	return modSize - tileSize - fmodf( pos, modSize );
}
