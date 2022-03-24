//
// Created by stephane bourque on 2022-03-11.
//

#pragma once

#include "framework/MicroService.h"
#include "RESTObjects/RESTAPI_AnalyticsObjects.h"
#include "StorageService.h"
#include "framework/orm.h"

namespace OpenWifi {

    template <typename DB> void ReturnRecordList(const char *ArrayName,DB & DBInstance, RESTAPIHandler & R) {
        Poco::JSON::Array   ObjArr;
        for(const auto &i:R.SelectedRecords()) {
            typename DB::RecordName     Rec;
            if(DBInstance.GetRecord("id",i,Rec)) {
                Poco::JSON::Object  Obj;
                Rec.to_json(Obj);
                ObjArr.add(Obj);
            } else {
                return R.BadRequest(RESTAPI::Errors::UnknownId + i);
            }
        }
        Poco::JSON::Object  Answer;
        Answer.set(ArrayName, ObjArr);
        return R.ReturnObject(Answer);
    }

    template <typename T> void MakeJSONObjectArray(const char * ArrayName, const std::vector<T> & V, RESTAPIHandler & R) {
        Poco::JSON::Array   ObjArray;
        for(const auto &i:V) {
            Poco::JSON::Object  Obj;
            i.to_json(Obj);
            ObjArray.add(Obj);
        }
        Poco::JSON::Object  Answer;
        Answer.set(ArrayName,ObjArray);
        return R.ReturnObject(Answer);
    }

    template<typename DB>
    void ListHandler(const char *BlockName, DB &DBInstance, RESTAPIHandler &R) {

        typedef typename DB::RecordVec RecVec;
        typedef typename DB::RecordName RecType;

        if (!R.QB_.Select.empty()) {
            return ReturnRecordList(BlockName, DBInstance, R);
        } else if (R.QB_.CountOnly) {
            Poco::JSON::Object Answer;
            auto C = DBInstance.Count();
            return R.ReturnCountOnly(C);
        } else {
            RecVec Entries;
            DBInstance.GetRecords(R.QB_.Offset, R.QB_.Limit, Entries);
            return MakeJSONObjectArray(BlockName, Entries, R);
        }
    }

}