// TODO:
// * Automatically correlate slide/jump.

/*

Copyright 2024 <>< Charels Lohr (cnlohr)

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the “Software”), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


*/

const targetWidth = 1280;
const targetHeight = 720;

// This gets populated at start
var pages = [ ];


var pageNumber = 0;
var pageSubNumber = 0;
var pageName = "";
var currentPage = null;

function getPageNumberFromID( id )
{
	for( var i = 0; i < pages.length; i++ )
	{
		if( pages[i].id == id )
			return i;
	}
}

function updatePage( num, subnum, warpView )
{
	if( num >= pages.length )
		num = pages.length-1;
	if( num < 0 )
		num = 0;

	if( currentPage && currentPage.onExit )
		eval( currentPage.onExit );

	pageNumber = num;
	pageSubNumber = subnum;
	const page = pages[pageNumber];
	currentPage = page;
	pageName = pages[pageNumber].id;
	const spstart = forceNumber( page.spStart );
	if( page.sp > spstart )
	{
		if( pageSubNumber < spstart ) pageSubNumber = spstart;
		if( pageSubNumber > page.sp ) pageSubNumber = page.sp; 
	}
	else
	{
		pageSubNumber = 0;
	}

	document.getElementById( page.view.e ) 

	var ofs = getOffsetOfElementRelativeToMainArea( document.getElementById( page.view.e ) );

	targS = page.view.s;
	targR = page.view.r;
	targX = page.view.x - ofs[0] * targS;
	targY = page.view.y - ofs[1] * targS;
	updateCanvasPosition( warpView );

	pageNumberIndicator.innerHTML = `.${pageSubNumber.toFixed(0)}/${pages.length}`;
	document.getElementById('pageSelect').value = Number(pageNumber);
	let bl10 = Math.log10( Number(pageNumber)+1.001) + 1;
	if( bl10 < 1 ) bl10 = 1;
	document.getElementById('pageSelect').style = `max-width:${Math.floor(bl10)*0.5+2.1}em`;

	// Deal with this page's PON's.
	var pid = page.alias ? page.alias : page.id;


	for( var i = 0|0; i <= (page.sp+20)|0; i++ )
	{
		var elms = document.querySelectorAll(`[pon="${pid}${i}"]`);
		for( var k = 0|0; k < elms.length; k++ )
		{
			let e = elms[k];
			e.style.visibility = (i<pageSubNumber) ? 'visible' : 'hidden';
		}

		var elms = document.querySelectorAll(`[poff="${pid}${i}"]`);
		for( var k = 0|0; k < elms.length; k++ )
		{
			let e = elms[k];
			if( i < pageSubNumber )
				e.style.visibility = 'hidden';
		}
	}

	if( page.onEntry )
		eval( page.onEntry );
}

function slidePrev()
{
	var pn = pageNumber;
	var psn = pageSubNumber - 1;
	if( psn < forceNumber( pages[pn].spStart ) )
	{
		pn--;
		if( pn < 0 ) pn = 0;
		psn = pages[pn].sp;
	}
	updatePage( pn, psn );
}

function slideNext()
{
	var pn = pageNumber;
	var psn = pageSubNumber + 1;

	if( psn > pages[pn].sp || !pages[pn].sp )
	{
		pn++;
		if( pn >= pages.length )
			pn = pages.length-1;
		else
			psn = 0;
	}
	updatePage( pn, psn );
}


function slidePrevFull()
{
	var pn = pageNumber - 1;
	if( pn < 0  )
		pn = 0;
	var psn = pages[pn].sp + 1;
	updatePage( pn, psn );
}

function slideNextFull()
{
	var pn = pageNumber + 1;
	if( pn >= pages.length )
		pn = pages.length-1;
	var psn = pages[pn].sp + 1;
	updatePage( pn, psn );
}


function showOverview()
{

	var ofs = getOffsetOfElementRelativeToMainArea( document.getElementById( 'innercontainer' ) );

	targS = 0.050;
	targR = 0;
	targX = -373.71 - ofs[0] * targS;
	targY = -282.10 - ofs[1] * targS;
	updateCanvasPosition( false );
}



var root = null;
var pageNumberIndicator = null;
var mainarea = null;

var frame = 0;
var time = 0;

var targX = 0, targY = 0, targR = 0, targS = 1;
var writeX = 0, writeY = 0;
var offsetter = null;
var shiftdown = false;
var ctrldown = false;
var mouseDownX = 0;
var mouseDownY = 0;
var lastMouseX = 0;
var lastMouseY = 0;
var mousePressed = false;
var mousePressedCtrl = false;
var mousePressedShift = false;
var mousePressedNone = false;

var showStatsAndView = false;

function UpdateStatsAndView()
{
	if( showStatsAndView )
	{
		var cw = root.clientWidth;
		var ch = root.clientHeight;
		document.getElementById("transformCopy").innerHTML = `{ r:${targR.toFixed(5)}, s:${targS.toFixed(3)}, x:${writeX.toFixed(2)}, y:${writeY.toFixed(2)}, e:'${offsetter}' }`;
		document.getElementById("screenStats").innerHTML = `${cw.toFixed(0)}x${ch.toFixed(0)}`;
	}
	else
	{
		document.getElementById("transformCopy").innerHTML = "";
		document.getElementById("screenStats").innerHTML = "A";
	}
}

function screenStatsClick()
{
	showStatsAndView = !showStatsAndView;
	UpdateStatsAndView();
}
var call = 0;

function animateFrame()
{
	time = performance.now();
	frame++;

	if( pageNumber >= 0 && pageNumber < pages.length )
	{
		let p = pages[pageNumber];
		if( p.animateFrame )
		{
			eval( p.animateFrame );
		}
	}
//	mainarea.style.transform = `rotate(${time*.01}deg)`;
//	mainarea.style.left = `${Math.sin(time*.01)*100}px`;
//	mainarea.style.backgroundColor = 'red';
	window.requestAnimationFrame( animateFrame );
}

// pxtransform( [x,y], inv ); // Goes from screen space pixels to content space pixels.
// pxtransform( [x,y], getMatrix() ); // Goes from content space pixels to screen space pixels.

function forceNumber( something )
{
	var ret = Number(something);
	if( ret >= 0 ) return ret;
	if( ret < 0 ) return ret;
	return 0;
}

function invertmat( matrix )
{
	// a = [ a b / c d ]
	// a^1 = 1/det(A) [ d -b / -c a ];
	var det = 1.0 / (matrix[0] * matrix[3] - matrix[1] * matrix[2]);
	var ret = [ matrix[3]*det, -matrix[1]*det, -matrix[2]*det, matrix[0]*det, 0, 0 ];
	var lx = -matrix[4];
	var ly = -matrix[5];
	ret[4] = ret[0] * lx + ret[2] * ly;
	ret[5] = ret[1] * lx + ret[3] * ly;
	return ret;
}

function pxtransform( xy, matrix )
{
	var ox = matrix[0] * xy[0] + matrix[2] * xy[1] + matrix[4];
	var oy = matrix[1] * xy[0] + matrix[3] * xy[1] + matrix[5];
	return [ ox, oy ];
}


function vxtransform( xy, matrix )
{
	var ox = matrix[0] * xy[0] + matrix[2] * xy[1];
	var oy = matrix[1] * xy[0] + matrix[3] * xy[1];
	return [ ox, oy ];
}

function updateStatus( str, emergency )
{
	document.getElementById("status").innerHTML = str;
	if( emergency )
		document.getElementById("status").style.text = "red";
	else
		document.getElementById("status").style.text = "";
}

function getOffsetOfElementRelativeToMainArea( el )
{
    var _x = 0;
    var _y = 0;
    while( el && el.id != "root" && !isNaN( el.offsetLeft ) && !isNaN( el.offsetTop ) ) {
        _x += el.offsetLeft - el.scrollLeft;
        _y += el.offsetTop - el.scrollTop;
        el = el.offsetParent;
    }

	// Disable any root scrolling.
	if( el && el.id == "root" )
	{
		el.scrollTop = 0;
		el.scrollLeft = 0;
	}

    return [ _x, _y ];
}

function getElementWithIdFromPoint( x, y )
{
	var e = document.elementFromPoint( x, y );
	while( e && e.id != "root" && !e.id && !isNaN( e.offsetLeft ) && !isNaN( e.offsetTop ) )
	{
        e = e.parentNode;
	}
	if( e )
		return e;
	else
		return null;
}


async function lateLoad()
{
	var elms = document.querySelectorAll(`[lateloadhtml]`);
	for( var k = 0|0; k < elms.length; k++ )
	{
		let e = elms[k];
		e.innerHTML = e.getAttribute( "lateloadhtml" );
	}
}

function onLoad()
{
	console.log( "OnLoad" );
	mainarea = document.getElementById( "mainarea" );
	root = document.getElementById( "root" );
	pageNumberIndicator = document.getElementById( "pageNumberIndicator" );

	var elms = document.querySelectorAll(`slide-descriptor`);
	for( var k = 0|0; k < elms.length; k++ )
	{
		let e = elms[k];
		const childRect = e.getBoundingClientRect();
		const op = e.offsetParent;
		let viewstr = e.getAttribute("view");
		console.log( viewstr );
		let viewv = eval( '(' + viewstr + ')' );
		pages.push( {
			title: e.getAttribute("title"),
			id: e.getAttribute("id"),
			alias: e.getAttribute("alias"),
			sp: Number(e.getAttribute("sp")),
			spStart: Number(e.getAttribute("spStart")),
			animateFrame: e.getAttribute("animateFrame"),
			onEntry: e.getAttribute("onEntry"),
			onExit: e.getAttribute("onExit"),
			view: viewv,
		} );

		var opt = document.createElement('option');
		opt.value = k;
		opt.innerHTML = (k+1) + "&nbsp;&nbsp;&nbsp;&nbsp;" + e.getAttribute("title");
		document.getElementById('pageSelect').appendChild( opt );
	}
	document.getElementById('pageSelect').onchange = function() { updatePage( Number( this.value )-1, 0, true ); }
	console.log( "Pages:" );
	console.log( pages );

	var jumps = document.querySelectorAll(`[slidejump]`);
	for( var k = 0|0; k < jumps.length; k++ )
	{
		let e = jumps[k];
		let index = getPageNumberFromID(e.getAttribute("slidejump"));
		e.addEventListener("dblclick", (event) => {
			var sel = window.getSelection();
	        sel.removeAllRanges();
			if( index == pageNumber )
				updatePage(index,pageSubNumber,false);
			else
				updatePage(index,0,false);
			event.preventDefault();
			event.stopPropagation();
		}, true );
	}


	window.requestAnimationFrame( animateFrame );
	document.body.addEventListener('keydown',keyDown);
	document.body.addEventListener('keyup',keyUp);
	document.body.addEventListener('mousedown',mouseDown);
	document.body.addEventListener('mouseup',mouseUp);
	document.body.addEventListener('mousemove',mouseMove);
	document.body.addEventListener('wheel',wheel);
	document.addEventListener('touchstart', e => {
		touchstartX = e.changedTouches[0].screenX;
		touchstartY = e.changedTouches[0].screenY;
	})
	document.addEventListener('touchend', e => {
		touchendX = e.changedTouches[0].screenX;
		touchendY = e.changedTouches[0].screenY;
		checkDirection();
	})

	addEventListener('resize',onResize);

//	setTimeout( () => {
		var defaultPageNumber = forceNumber(localStorage.getItem("pageNumber", pageNumber));
		var defaultSubNumber = forceNumber(localStorage.getItem("pageSubNumber", pageSubNumber));
		updatePage( defaultPageNumber, defaultSubNumber, true );
		if( localStorage.getItem("targR") )
		{
			targR = forceNumber(localStorage.getItem("targR"));
			targS = forceNumber(localStorage.getItem("targS"));
			targX = forceNumber(localStorage.getItem("targX"));
			targY = forceNumber(localStorage.getItem("targY"));
		}
		updateCanvasPosition(true);
//	}, 10 );

	lateLoad();
}

function aspectScale()
{
	var cw = root.clientWidth;
	var ch = root.clientHeight;
	var aw = cw/targetWidth;
	var ah = ch/targetHeight;
	return (aw<ah)?aw:ah;
}

function getMatrix()
{
	var cw = root.clientWidth;
	var ch = root.clientHeight;
	var useS = targS;
	var sscale = aspectScale();

	var useX = (targX*sscale+cw/2);
	var useY = (targY*sscale+ch/2);

	useS *= sscale;

	var matrix = [ Math.cos( targR ) * useS, Math.sin( targR ) * useS,
				  -Math.sin( targR ) * useS, Math.cos( targR ) * useS, useX, useY];
	return matrix;
}

function limitScale()
{
	if( targS < 0.05 ) targS = 0.05;
	if( targS > 20 ) targS = 20;
}

function resolvePositionForCopy()
{
	var cw = root.clientWidth;
	var ch = root.clientHeight;
	writeX = targX/targS;
	writeY = targY/targS;
	const tE = getElementWithIdFromPoint( cw/2, ch/2 );
	offsetter = "";
	if( tE && tE.id != "root" )
	{
		offsetter = tE.id;
		const offsets = getOffsetOfElementRelativeToMainArea( tE );
		writeX += offsets[0];
		writeY += offsets[1];
	}

	writeX *= targS;
	writeY *= targS;

	UpdateStatsAndView();

	localStorage.setItem("targR", targR);
	localStorage.setItem("targS", targS);
	localStorage.setItem("targX", targX);
	localStorage.setItem("targY", targY);
	localStorage.setItem("pageNumber", pageNumber);
	localStorage.setItem("pageSubNumber", pageSubNumber);
}

function updateCanvasPosition( immediate )
{
	var cw = root.clientWidth;
	var ch = root.clientHeight;
	if( immediate == true )
		mainarea.style.transition = '';
	else
		mainarea.style.transition = 'transform .5s ease-out';

	mainarea.style.transform = `matrix( ${getMatrix()} )`;

	setTimeout( ()=>{resolvePositionForCopy()}, 600 );
}

function onResize()
{
	updateCanvasPosition( true );
}

function transformCopyClick(e)
{
	document.execCommand('copy'); // or 'cut'
}

function mouseDown( e )
{
	if( e && e.target && (e.target.type == "text" || e.target.type == "textarea") )
		return;
	var x = e.clientX;
	var y = e.clientY;
	lastMouseX = mouseDownX = x;
	lastMouseY = mouseDownY = y;
	mousePressed = true;
	mousePressedCtrl = ctrldown;
	mousePressedShift = shiftdown;
	runningR = targR;
	runningS = targS;
}

function mouseUp( e )
{
	mousePressed = false;
}

function mouseMove( e )
{
	if( e && e.target && (e.target.type == "text" || e.target.type == "textarea") )
		return;

	var cw = root.clientWidth;
	var ch = root.clientHeight;
	var hw = cw/2;
	var hh = ch/2;
	var x = e.clientX;
	var y = e.clientY;

	if( !mousePressed && e.buttons )
	{
		mouseDown( e );
	}
	if( mousePressed && !e.buttons )
	{
		mousePressed = false;
		updateCanvasPosition();
	}

	if( mousePressed )
	{
		if( e.ctrlKey )
		{
			var pre_scroll_content_space = pxtransform( [hw, hh], invertmat( getMatrix() ) );
			var s0 = Math.sqrt( Math.pow(lastMouseX - hw, 2.0) + Math.pow(lastMouseY - hh,2.0) );
			var s1 = Math.sqrt( Math.pow(x - hw,2.0) + Math.pow(y - hh,2.0) );
			var d0 = Math.atan2( lastMouseX - hw, lastMouseY - hh );
			var d1 = Math.atan2( x - hw, y - hh );
			if( s0 > 10 && s1 > 10 )
			{
				runningR -= d1-d0;
				//targR = Math.round( (runningR /3.14159 * 180)/10) * 10 * 3.14159 / 180.0;

				targS *= s1/s0;
			}
			var post_scroll_pixel_space = pxtransform( pre_scroll_content_space, getMatrix() );
			var disagreement = [hw - post_scroll_pixel_space[0], hh - post_scroll_pixel_space[1]];
			targX += disagreement[0] / aspectScale();
			targY += disagreement[1] / aspectScale();

			updateCanvasPosition(true);
		}
		else if( e.shiftKey )
		{
		}
		else
		{
			let ar = aspectScale();
			targX += (x - lastMouseX) / ar;
			targY += (y - lastMouseY) / ar;
			updateCanvasPosition(true);
		}
	}

	lastMouseX = x;
	lastMouseY = y;
}

function zoominout( delta, mx, my )
{
	var cw = root.clientWidth;
	var ch = root.clientHeight;

	// pre_scroll_content_space is the mouse coordinate on the base canvas. (Verified correct)
	var pre_scroll_content_space = pxtransform( [mx, my], invertmat( getMatrix() ) );

	var targSbefore = targS;
	targS *= Math.pow( 1.0015, delta );
	limitScale();

	var post_scroll_pixel_space = pxtransform( pre_scroll_content_space, getMatrix() );
	
	var disagreement = [mx - post_scroll_pixel_space[0], my - post_scroll_pixel_space[1]];
	targX += disagreement[0] / aspectScale();
	targY += disagreement[1] / aspectScale();
	updateCanvasPosition();
}

let touchstartX = 0;
let touchstartY = 0;
let touchendX = 0
let touchendY = 0
    
function checkDirection() {
	// Swiping
	// Make swipe left do next, swipe-right do prev
	// Make swipe up do next and swipe down do prev
	if( touchendX+touchendY > touchstartX+touchstartY ) slidePrev();
	if( touchendX+touchendY < touchstartX+touchstartY ) slideNext();
}

function wheel( e )
{
	if( e.shiftKey )
	{
		zoominout(e.wheelDeltaY, e.clientX, e.clientY);
	}
	else
	{
		if( e.wheelDeltaY > 0 ) slidePrev();
		if( e.wheelDeltaY < 0 ) slideNext();
	}
}

function keyUp( key )
{
	switch( key.keyCode )
	{
		case 17: ctrldown = false; break;
		case 16: shiftdown = false; break;
	}

	if( key.target != document.body ) return;
}

function keyDown( key )
{
	switch( key.keyCode )
	{
		case 17: ctrldown = true; break;
		case 16: shiftdown = true; break;
	}

	if( key.target != document.body ) return;

	switch( key.keyCode )
	{
		case 8:  case 80: case 38: case 37: slidePrev(); break;
		case 32: case 78: case 40: case 39: slideNext(); break;
		case 33: slidePrev(); break; // slidePrevFull(); break;
		case 34: slideNext(); break; // slideNextFull(); break;
		case 36: updatePage( 0, 0 ); break;
		case 35: updatePage( pages.length-1, 999 ); break;
//		case 37: targX -= 1; break;
//		case 39: targX += 1; break;
//		case 38: targY -= 1; break;
//		case 40: targY += 1; break;
		case 87: targY += 100; break;
		case 83: targY -= 100; break;
		case 65: targX += 100; break;
		case 68: targX -= 100; break;
		case 109: zoominout( -200, lastMouseX, lastMouseY ); break;
		case 107: zoominout(  200, lastMouseX, lastMouseY ); break;
		default: 	console.log( key.keyCode ); break;
	}
	updateCanvasPosition();
}



