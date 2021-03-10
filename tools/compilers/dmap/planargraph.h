#pragma once

#include "earcut.h"

class PlanarGraph {
public:
	struct SeamEdge {
		int v[2];
	};

private:
	struct Vertex {
		idVec2 pos;
		int outBeg, outEnd;
		int inComp;		//index in components[]
	};
	struct Coedge {
		int v[2];		//indices in verts[]: 0 - start, 1 - end
		int outIdx;		//index in outgoing[]
		idVec2d vec;	//(for polar sort)
		int inFacet;	//index in facets[]
	};
	struct Edge {

	};
	struct Facet {
		int rightmost;			//index in verts[]
		bool clockwise;			//true for "inner" loop, false for "outer" loop
		int parent;				//index in facets[]: who immediately contains it?
		int inComp;				//index in components[]
		idVec2 bounds[2];		//[min/max]
		double area;			//doubled, positive when CCW
		int fcBeg, fcEnd;
		int holeBeg, holeEnd;
	};
	struct Component {
		int v;					//index in verts[]: any vertex
		int cwFacet;			//index in facets[]: the only clockwise
		int parent;				//index in components[]: who immediately contains it
	};

	idList<Vertex> verts;
	idList<Edge> edges;
	//edges[k] consists of two opposite coedges/halfedges:
	//  coedges[2*k] and coedges[2*k+1]
	idList<Coedge> coedges;
	//outgoing coedges from vertex v are:
	//  coedges[outgoing[i]]   for i in [verts[v].outBeg...verts[v].outEnd)
	idList<int> outgoing;

	//connected components
	idList<Component> components;

	idList<Facet> facets;
	//facets[k] consists of coedges:
	//  coedges[facetCoedges[i]]   for i in [facets[f].fcBeg...facets[f].fcEnd)
	idList<int> facetCoedges;	//indices in coedges
	//facets[k] has hole facets:
	//  facets[facetHoles[i]]   for i in [facets[f].holeBeg...facets[f].holeEnd)
	idList<int> facetHoles;


	//temporary storage
	idList<int> _work;
	EarCutter earcut;

public:

	//step 1: fill graph
	int AddVertex(idVec2 pos);
	int AddEdge(int v0, int v1);

	//step 2: finish graph
	void Finish();

	//step 3: find all faces
	void BuildFaces();

	typedef EarCutter::Triangle Triangle;
	typedef EarCutter::SeamEdge AddedEdge;
	//step 4: fill all faces with triangles
	void TriangulateFaces(idList<Triangle> &tris, idList<AddedEdge> &edges);

	//optional: restart from scratch without losing memory buffers
	void Reset();
};
