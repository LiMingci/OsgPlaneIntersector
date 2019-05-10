#pragma once

#include <map>

#include <osg/Vec4d>
#include <osg/Notify>
#include <osg/io_utils>

#include "IsoheightDef.h"

ISOHEIGHT_NAMESPACE_BEGIN

struct RefPolyline : public osg::Referenced
{
	typedef std::vector<osg::Vec4d> Polyline;
	Polyline _polyline;

	void reverse()
	{
		unsigned int s = 0;
		unsigned int e = _polyline.size() - 1;
		for (; s<e; ++s, --e)
		{
			std::swap(_polyline[s], _polyline[e]);
		}

	}
};

struct CompKey
{
	bool operator()(const osg::Vec4d &k1, const osg::Vec4d &k2) const
	{
		osg::Vec4d sub = k1 - k2;
		if (sub.length2() < 1e-6) return false;

		if (k1[0] < k2[0]) return true;
		else if (k1[0] > k2[0]) return false;
		else if (k1[1] < k2[1]) return true;
		else if (k1[1] > k2[1]) return false;
		else if (k1[2] < k2[2]) return true;
		else if (k1[2] > k2[2]) return false;
		else return (k1[3] < k2[3]);
	}

};

class PolylineConnector
{
public:

	typedef std::map<osg::Vec4d, osg::ref_ptr<RefPolyline>, CompKey>      PolylineMap;
	//typedef std::vector<std::pair<osg::Vec4d, osg::ref_ptr<RefPolyline>> >      PolylineMap;
	typedef std::vector< osg::ref_ptr<RefPolyline> >                            PolylineList;

	PolylineList                               _polylines;
	PolylineMap                                _startPolylineMap;
	PolylineMap                                _endPolylineMap;


	void add(const osg::Vec3d& v1, const osg::Vec3d& v2)
	{
		add(osg::Vec4d(v1, 0.0), osg::Vec4d(v2, 0.0));
	}

	void add(const osg::Vec4d& v1, const osg::Vec4d& v2)
	{
		osg::Vec4d subVec = v1 - v2;
		if (subVec.length2() < 1e-6) return;
		//if (v1 == v2) return;

		PolylineMap::iterator v1_start_itr = _startPolylineMap.find(v1);
		PolylineMap::iterator v1_end_itr = _endPolylineMap.find(v1);

		PolylineMap::iterator v2_start_itr = _startPolylineMap.find(v2);
		PolylineMap::iterator v2_end_itr = _endPolylineMap.find(v2);

		unsigned int v1_connections = 0;
		if (v1_start_itr != _startPolylineMap.end()) ++v1_connections;
		if (v1_end_itr != _endPolylineMap.end()) ++v1_connections;

		unsigned int v2_connections = 0;
		if (v2_start_itr != _startPolylineMap.end()) ++v2_connections;
		if (v2_end_itr != _endPolylineMap.end()) ++v2_connections;

		if (v1_connections == 0) // v1 is no connected to anything.
		{
			if (v2_connections == 0)
			{
				// new polyline
				newline(v1, v2);
			}
			else if (v2_connections == 1)
			{
				// v2 must connect to either a start or an end.
				if (v2_start_itr != _startPolylineMap.end())
				{
					insertAtStart(v1, v2_start_itr);
				}
				else if (v2_end_itr != _endPolylineMap.end())
				{
					insertAtEnd(v1, v2_end_itr);
				}
				else
				{
					OSG_NOTICE << "Error: should not get here!" << std::endl;
				}
			}
			else // if (v2_connections==2)
			{
				// v2 connects to a start and an end - must have a loop in the list!
				OSG_NOTICE << "v2=" << v2 << " must connect to a start and an end - must have a loop!!!!!." << std::endl;
			}
		}
		else if (v2_connections == 0) // v1 is no connected to anything.
		{
			if (v1_connections == 1)
			{
				// v1 must connect to either a start or an end.
				if (v1_start_itr != _startPolylineMap.end())
				{
					insertAtStart(v2, v1_start_itr);
				}
				else if (v1_end_itr != _endPolylineMap.end())
				{
					insertAtEnd(v2, v1_end_itr);
				}
				else
				{
					OSG_NOTICE << "Error: should not get here!" << std::endl;
				}
			}
			else // if (v1_connections==2)
			{
				// v1 connects to a start and an end - must have a loop in the list!
				OSG_NOTICE << "v1=" << v1 << " must connect to a start and an end - must have a loop!!!!!." << std::endl;
			}
		}
		else
		{
			// v1 and v2 connect to existing lines, now need to fuse them together.
			bool v1_connected_to_start = v1_start_itr != _startPolylineMap.end();
			bool v1_connected_to_end = v1_end_itr != _endPolylineMap.end();

			bool v2_connected_to_start = v2_start_itr != _startPolylineMap.end();
			bool v2_connected_to_end = v2_end_itr != _endPolylineMap.end();

			if (v1_connected_to_start)
			{
				if (v2_connected_to_start)
				{
					fuse_start_to_start(v1_start_itr, v2_start_itr);
				}
				else if (v2_connected_to_end)
				{
					fuse_start_to_end(v1_start_itr, v2_end_itr);
				}
				else
				{
					OSG_NOTICE << "Error: should not get here!" << std::endl;
				}
			}
			else if (v1_connected_to_end)
			{
				if (v2_connected_to_start)
				{
					fuse_start_to_end(v2_start_itr, v1_end_itr);
				}
				else if (v2_connected_to_end)
				{
					fuse_end_to_end(v1_end_itr, v2_end_itr);
				}
				else
				{
					OSG_NOTICE << "Error: should not get here!" << std::endl;
				}
			}
			else
			{
				OSG_NOTICE << "Error: should not get here!" << std::endl;
			}
		}
	}

	void newline(const osg::Vec4d& v1, const osg::Vec4d& v2)
	{
		RefPolyline* polyline = new RefPolyline;
		polyline->_polyline.push_back(v1);
		polyline->_polyline.push_back(v2);
		_startPolylineMap[v1] = polyline;
		_endPolylineMap[v2] = polyline;
	}

	void insertAtStart(const osg::Vec4d& v, PolylineMap::iterator v_start_itr)
	{
		// put v1 at the start of its poyline
		RefPolyline* polyline = v_start_itr->second.get();
		polyline->_polyline.insert(polyline->_polyline.begin(), v);

		// reinsert the polyine at the new v1 end
		_startPolylineMap[v] = polyline;

		// remove the original entry
		_startPolylineMap.erase(v_start_itr);

	}

	void insertAtEnd(const osg::Vec4d& v, PolylineMap::iterator v_end_itr)
	{
		// put v1 at the end of its poyline
		RefPolyline* polyline = v_end_itr->second.get();
		polyline->_polyline.push_back(v);

		// reinsert the polyine at the new v1 end
		_endPolylineMap[v] = polyline;

		// remove the original entry
		_endPolylineMap.erase(v_end_itr);

	}

	void fuse_start_to_start(PolylineMap::iterator start1_itr, PolylineMap::iterator start2_itr)
	{
		osg::ref_ptr<RefPolyline> poly1 = start1_itr->second;
		osg::ref_ptr<RefPolyline> poly2 = start2_itr->second;

		PolylineMap::iterator end1_itr = _endPolylineMap.find(poly1->_polyline.back());
		PolylineMap::iterator end2_itr = _endPolylineMap.find(poly2->_polyline.back());

		// clean up the iterators associated with the original polylines
		_startPolylineMap.erase(start1_itr);
		_startPolylineMap.erase(start2_itr);
		_endPolylineMap.erase(end1_itr);
		_endPolylineMap.erase(end2_itr);

		// reverse the first polyline
		poly1->reverse();

		// add the second polyline to the first
		poly1->_polyline.insert(poly1->_polyline.end(),
			poly2->_polyline.begin(), poly2->_polyline.end());

		_startPolylineMap[poly1->_polyline.front()] = poly1;
		_endPolylineMap[poly1->_polyline.back()] = poly1;

	}

	void fuse_start_to_end(PolylineMap::iterator start_itr, PolylineMap::iterator end_itr)
	{
		osg::ref_ptr<RefPolyline> end_poly = end_itr->second;
		osg::ref_ptr<RefPolyline> start_poly = start_itr->second;

		PolylineMap::iterator end_start_poly_itr = _endPolylineMap.find(start_poly->_polyline.back());

		// add start_poly to end of end_poly
		end_poly->_polyline.insert(end_poly->_polyline.end(),
			start_poly->_polyline.begin(), start_poly->_polyline.end());

		// reassign the end of the start poly so that it now points to the merged end_poly
		end_start_poly_itr->second = end_poly;


		// remove entries for the end of the end_poly and the start of the start_poly
		_endPolylineMap.erase(end_itr);
		_startPolylineMap.erase(start_itr);

		if (end_poly == start_poly)
		{
			_polylines.push_back(end_poly);
		}

	}

	void fuse_end_to_end(PolylineMap::iterator end1_itr, PolylineMap::iterator end2_itr)
	{

		// return;

		osg::ref_ptr<RefPolyline> poly1 = end1_itr->second;
		osg::ref_ptr<RefPolyline> poly2 = end2_itr->second;

		PolylineMap::iterator start1_itr = _startPolylineMap.find(poly1->_polyline.front());
		PolylineMap::iterator start2_itr = _startPolylineMap.find(poly2->_polyline.front());

		// clean up the iterators associated with the original polylines
		_startPolylineMap.erase(start1_itr);
		_startPolylineMap.erase(start2_itr);
		_endPolylineMap.erase(end1_itr);
		_endPolylineMap.erase(end2_itr);

		// reverse the first polyline
		poly2->reverse();

		// add the second polyline to the first
		poly1->_polyline.insert(poly1->_polyline.end(),
			poly2->_polyline.begin(), poly2->_polyline.end());

		_startPolylineMap[poly1->_polyline.front()] = poly1;
		_endPolylineMap[poly1->_polyline.back()] = poly1;

	}

	void consolidatePolylineLists()
	{
		// move the remaining open ended line segments into the polyline list
		for (PolylineMap::iterator sitr = _startPolylineMap.begin();
			sitr != _startPolylineMap.end();
			++sitr)
		{
			_polylines.push_back(sitr->second);
		}
	}

	void report()
	{
		OSG_NOTICE << "report()" << std::endl;

		OSG_NOTICE << "start:" << std::endl;
		for (PolylineMap::iterator sitr = _startPolylineMap.begin();
			sitr != _startPolylineMap.end();
			++sitr)
		{
			OSG_NOTICE << "  line - start = " << sitr->first << " polyline size = " << sitr->second->_polyline.size() << std::endl;
		}

		OSG_NOTICE << "ends:" << std::endl;
		for (PolylineMap::iterator eitr = _endPolylineMap.begin();
			eitr != _endPolylineMap.end();
			++eitr)
		{
			OSG_NOTICE << "  line - end = " << eitr->first << " polyline size = " << eitr->second->_polyline.size() << std::endl;
		}

		for (PolylineList::iterator pitr = _polylines.begin();
			pitr != _polylines.end();
			++pitr)
		{
			OSG_NOTICE << "polyline:" << std::endl;
			RefPolyline::Polyline& polyline = (*pitr)->_polyline;
			for (RefPolyline::Polyline::iterator vitr = polyline.begin();
				vitr != polyline.end();
				++vitr)
			{
				OSG_NOTICE << "  " << *vitr << std::endl;
			}
		}


	}

	void fuse()
	{
		OSG_NOTICE << "supposed to be doing a fuse..." << std::endl;
	}

};

struct TriangleIntersector
{

	osg::Plane                              _plane;

	PolylineConnector _polylineConnector;

	TriangleIntersector() {}

	void set(const osg::Plane& plane)
	{
		_plane = plane;
	}

	inline double distance(const osg::Plane& plane, const osg::Vec4d& v) const
	{
		return plane[0] * v.x() +
			plane[1] * v.y() +
			plane[2] * v.z() +
			plane[3];
	}

	inline void add(osg::Vec4d& vs, osg::Vec4d& ve)
	{
		_polylineConnector.add(vs, ve);
	}

	inline void operator () (const osg::Vec3& v1, const osg::Vec3& v2, const osg::Vec3& v3, bool)
	{
		double eps = 1e-6;

		double d1 = _plane.distance(v1);
		//if (d1 == 0.0)
		//{
		//	osg::Vec3 v1_eps = v1 + _plane.getNormal() * eps;
		//	d1 += eps;
		//}
		double d2 = _plane.distance(v2);
		//if (d2 == 0.0)
		//{
		//	osg::Vec3 v2_eps = v2 + _plane.getNormal() * eps;
		//	d2 += eps;
		//}
		double d3 = _plane.distance(v3);
		//if (d3 == 0.0)
		//{
		//	osg::Vec3 v3_eps = v3 + _plane.getNormal() * eps;
		//	d3 += eps;
		//}
			

		unsigned int numBelow = 0;
		unsigned int numAbove = 0;
		unsigned int numOnPlane = 0;
		if (d1<0) ++numBelow;
		else if (d1>0) ++numAbove;
		else ++numOnPlane;

		if (d2<0) ++numBelow;
		else if (d2>0) ++numAbove;
		else ++numOnPlane;

		if (d3<0) ++numBelow;
		else if (d3>0) ++numAbove;
		else ++numOnPlane;

		
		osg::Vec3 v1_eps(v1);
		osg::Vec3 v2_eps(v2);
		osg::Vec3 v3_eps(v3);
		

		// trivially discard triangles that are completely one side of the plane
		if (numAbove == 3 || numBelow == 3) return;

		if (numOnPlane == 3)
		{
			// triangle lives wholy in the plane
			OSG_NOTICE << "3" << std::endl;
			return;
		}

		if (numOnPlane == 2)
		{
			// one edge lives wholy in the plane
			OSG_NOTICE << "2" << std::endl;
			if (d1 == 0.0 && d2 == 0.0)
			{
				if (d3 > 0.0)
				{
					v1_eps = v1 - _plane.getNormal() * eps;
					v2_eps = v2 - _plane.getNormal() * eps;
					d1 -= eps;
					d2 -= eps;
				}
				else
				{
					v1_eps = v1 + _plane.getNormal() * eps;
					v2_eps = v2 + _plane.getNormal() * eps;
					d1 += eps;
					d2 += eps;
				}
			}

			if (d1 == 0.0 && d3 == 0.0)
			{
				if (d2 > 0.0)
				{
					v1_eps = v1 - _plane.getNormal() * eps;
					v3_eps = v3 - _plane.getNormal() * eps;
					d1 -= eps;
					d3 -= eps;
				}
				else
				{
					v1_eps = v1 + _plane.getNormal() * eps;
					v3_eps = v3 + _plane.getNormal() * eps;
					d1 += eps;
					d3 += eps;
				}
			}

			if (d2 == 0.0 && d3 == 0.0)
			{
				if (d1 > 0.0)
				{
					v2_eps = v2 - _plane.getNormal() * eps;
					v3_eps = v3 - _plane.getNormal() * eps;
					d2 -= eps;
					d3 -= eps;
				}
				else
				{
					v2_eps = v2 + _plane.getNormal() * eps;
					v3_eps = v3 + _plane.getNormal() * eps;
					d2 += eps;
					d3 += eps;
				}
			}
		}

		//if (numOnPlane == 1)
		//{
		//	// one point lives wholy in the plane
		//	OSG_NOTICE << "1" << std::endl;
		//	return;
		//}

		osg::Vec4d v[2];
		unsigned int numIntersects = 0;

		osg::Vec4d p1(v1_eps, v1_eps.z());
		osg::Vec4d p2(v2_eps, v2_eps.z());
		osg::Vec4d p3(v3_eps, v3_eps.z());

		if (d1*d2 < 0.0)
		{
			// edge 12 itersects
			double div = 1.0 / (d2 - d1);
			v[numIntersects++] = p1* (d2*div) - p2 * (d1*div);
		}

		if (d2*d3 < 0.0)
		{
			// edge 23 itersects
			double div = 1.0 / (d3 - d2);
			v[numIntersects++] = p2* (d3*div) - p3 * (d2*div);
		}

		if (d1*d3 < 0.0)
		{
			if (numIntersects<2)
			{
				// edge 13 itersects
				double div = 1.0 / (d3 - d1);
				v[numIntersects++] = p1* (d3*div) - p3 * (d1*div);
			}
			else
			{
				OSG_NOTICE << "!!! too many intersecting edges found !!!" << std::endl;
			}

		}

		add(v[0], v[1]);

	}

};







ISOHEIGHT_NAMESPACE_END
