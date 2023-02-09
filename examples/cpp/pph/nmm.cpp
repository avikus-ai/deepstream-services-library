#include <iostream>
#include <vector>
#include <algorithm>
#include <string>
#include <unordered_map>

#include "xtensor/xarray.hpp"
#include "xtensor/xio.hpp"
#include "xtensor/xview.hpp"
#include "xtensor/xadapt.hpp"
#include "xtensor/xsort.hpp"
#include "xtensor/xindex_view.hpp"

using namespace std;
typedef xt::xarray<double> xarr;

xarr index_select(const xarr& a, const xarr& order)
{
    int n = (int)(xt::adapt(order.shape())[0]);
    // int b = (int)(xt::adapt(a.shape())[1]);
    // cout << "in the func (xt::adapt(order.shape())[0]) => " << (xt::adapt(order.shape())[0]) << endl;
    // cout << "in the func (xt::adapt(a.shape())[1]) => " << (xt::adapt(a.shape())[1]) << endl;
    xarr out = xt::zeros<double>({n});
    double idx = 0.0;

    for (auto it = order.begin(); it != order.end(); ++it)
    {
        xt::view(out, idx++, xt::all()) = xt::view(a, *it, xt::all());
    }

    return out;
}

std::unordered_map<int, vector<vector<int>>> greedy_nmm(
// void greedy_nmm(
    const xarr& object_predictions,
    // const char*& match_metric,
    const float& match_threshold
)
{   
    // cout << "object predictions are => \n" << object_predictions << endl;

    std::unordered_map<int, vector<vector<int>>> keep_to_merge_list;

    xarr x1 = xt::view(object_predictions, xt::all(), 0);
    xarr y1 = xt::view(object_predictions, xt::all(), 1);
    xarr x2 = xt::view(object_predictions, xt::all(), 2);
    xarr y2 = xt::view(object_predictions, xt::all(), 3);
    
    xarr scores = xt::view(object_predictions, xt::all(), 4);

    xarr areas = (x2 - x1) * (y2 - y1);

    xarr order = xt::argsort(scores);
    // cout << "first order" << order << endl;

    vector<int> keep;

    while (xt::any(order))
    {
        int idx = (int)(order(xt::adapt(order.shape())[0] - 1));
        // cout << "idx" << idx << endl;

        keep.push_back(idx);

        order = xt::view(order, xt::range(0, xt::adapt(order.shape())[0] - 1));
        // cout << "later order" << order << endl;

        if (!xt::any(order))
        {
            break;
        }

        // cout << "x1" << x1 << endl;
        // cout << "x2" << x2 << endl;
        // cout << "y1" << y1 << endl;
        // cout << "y2" << y2 << endl;

        // xarr xx1 = xt::view(x1, xt::range(0, xt::adapt(order.shape())[0]), xt::all());
        // xarr xx2 = xt::view(x2, xt::range(0, xt::adapt(order.shape())[0]), xt::all());
        // xarr yy1 = xt::view(y1, xt::range(0, xt::adapt(order.shape())[0]), xt::all());
        // xarr yy2 = xt::view(y2, xt::range(0, xt::adapt(order.shape())[0]), xt::all());

        xarr xx1 = index_select(x1, order);
        xarr xx2 = index_select(x2, order);
        xarr yy1 = index_select(y1, order);
        xarr yy2 = index_select(y2, order);        

        // cout << "xx1" << xx1 << endl;
        // cout << "xx2" << xx2 << endl;
        // cout << "yy1" << yy1 << endl;
        // cout << "yy2" << yy2 << endl;

        // cout << "x1(idx)" << x1(idx) << endl;
        // cout << "y1(idx)" << y1(idx) << endl;
        // cout << "x2(idx)" << x2(idx) << endl;
        // cout << "y2(idx)" << y2(idx) << endl;

        xx1 = xt::maximum(xx1, x1(idx));
        yy1 = xt::maximum(yy1, y1(idx));
        xx2 = xt::minimum(xx2, x2(idx));
        yy2 = xt::minimum(yy2, y2(idx));

        // cout << "xx1" << xx1 << endl;
        // cout << "xx2" << xx2 << endl;
        // cout << "yy1" << yy1 << endl;
        // cout << "yy2" << yy2 << endl;

        xarr w = xx2 - xx1;
        xarr h = yy2 - yy1;

        // cout << "w" << w << endl;
        // cout << "h" << h << endl;

        w = xt::clip(w, 0.0, __FLT_MAX__);
        h = xt::clip(h, 0.0, __FLT_MAX__);

        // cout << "w" << w << endl;
        // cout << "h" << h << endl;

        xarr inter = w * h;
        // cout << "inter" << inter << endl;

        // cout << "areas" << areas << endl;
        xarr rem_areas = index_select(areas, order);
        // cout << "rem_areas" << rem_areas << endl;

        xarr smaller = xt::minimum(rem_areas, areas(idx));
        // cout << "smaller" << smaller << endl;

        xarr match_metric_value = inter / smaller;
        // cout << "match_metric_value" << match_metric_value << endl;

        xt::xarray<bool> unmatched_mask = match_metric_value < match_threshold;
        xt::xarray<bool> matched_mask = match_metric_value >= match_threshold;
        xarr matched_box_indices = xt::filter(order, matched_mask);
        // cout << "unmatched_mask" << unmatched_mask << endl;
        // cout << "matched_mask" << matched_mask << endl;
        // cout << "matched_box_indices" << matched_box_indices << endl;

        order = xt::filter(order, unmatched_mask);
        // cout << "new order" << order << endl;

        // xt::view(order, 0, 0).data

        // cout << "about to push matched box indicies of idx =>" << idx << endl;
        // cout << matched_box_indices << endl;
        // cout << x1 << endl;

        if (xt::adapt(matched_box_indices.shape())[0] == 0)
        {
            // cout << "ther is no matching indicies" << endl;

            int x1_idx = (int)(x1(idx));
            int y1_idx = (int)(y1(idx));
            int x2_idx = (int)(x2(idx));
            int y2_idx = (int)(y2(idx));

            keep_to_merge_list[idx].push_back(vector<int>{x1_idx, y1_idx, x2_idx, y2_idx, idx});

            // cout << "for shouldn't be running" << endl;
        }
        
        for (size_t i = 0; i < xt::adapt(matched_box_indices.shape())[0]; ++i)
        {
            // auto matched_box_idx = xt::view(matched_box_indices, i, xt::all());
            int matched_box_idx = (int)matched_box_indices(i);
            // cout << matched_box_idx << endl;

            // int matched_box_iidx = (int)matched_box_idx;
            // cout << matched_box_iidx << endl;

            int x1_idx = (int)(x1(matched_box_idx));
            int y1_idx = (int)(y1(matched_box_idx));
            int x2_idx = (int)(x2(matched_box_idx));
            int y2_idx = (int)(y2(matched_box_idx));

            // auto x1_idx = (xt::view(x1, matched_box_idx));
            // auto y1_idx = (xt::view(y1, matched_box_idx));
            // auto x2_idx = (xt::view(x2, matched_box_idx));
            // auto y2_idx = (xt::view(y2, matched_box_idx));

            // cout << x1_idx << endl;
            // cout << y1_idx << endl;
            // cout << x2_idx << endl;
            // cout << y2_idx << endl;

            // int x1_iidx = (int)(*(x1_idx.data()));
            // int y1_iidx = (int)(*(y1_idx.data()));
            // int x2_iidx = (int)(*(x2_idx.data()));
            // int y2_iidx = (int)(*(y2_idx.data()));

            keep_to_merge_list[idx].push_back(vector<int>{x1_idx, y1_idx, x2_idx, y2_idx, matched_box_idx});

            // cout << "end of for loop" << endl;
            // keep_to_merge_list[idx].push_back((int)*it);
        }
        // cout << "end of pushing matched box indicies" << endl;

        // cout << "beginning of checking keep to merge list of idx => " << idx << endl;
        // for (auto elem : keep_to_merge_list)
        // {
        //     cout << elem.first << endl;
        //     vector<vector<int>> tmp = elem.second;

        //     for (size_t i = 0; i < tmp.size(); ++i)
        //     {
        //         for (size_t j = 0; j < tmp[i].size(); ++j)
        //         {
        //             cout << tmp[i][j] << ' ';
        //         }
        //         cout << endl;
        //     }
        
        //     cout << endl;
        
        //     // cout << endl;
        // }
        // cout << "end of checking keep to merge list" << endl;
        // cout << endl;
        // cout << endl;
    }

    return keep_to_merge_list;
}