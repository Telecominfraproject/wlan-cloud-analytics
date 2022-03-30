//
// Created by stephane bourque on 2022-03-11.
//

#include "RESTAPI_board_handler.h"
#include "VenueCoordinator.h"

namespace OpenWifi {
    void RESTAPI_board_handler::DoGet() {
        auto id = GetBinding("id","");
        if(id.empty()) {
            return BadRequest(RESTAPI::Errors::MissingUUID);
        }

        AnalyticsObjects::BoardInfo B;
        if(!StorageService()->BoardsDB().GetRecord("id",id,B)) {
            return NotFound();
        }

        Poco::JSON::Object  Answer;
        B.to_json(Answer);
        return ReturnObject(Answer);
    }

    void RESTAPI_board_handler::DoDelete() {
        auto id = GetBinding("id","");
        if(id.empty()) {
            return BadRequest(RESTAPI::Errors::MissingUUID);
        }

        AnalyticsObjects::BoardInfo B;
        if(!StorageService()->BoardsDB().GetRecord("id",id,B)) {
            return NotFound();
        }

        VenueCoordinator()->StopBoard(id);
        if(!StorageService()->BoardsDB().DeleteRecord("id",id)) {
            return NotFound();
        }
        StorageService()->TimePointsDB().DeleteBoard(id);

        return OK();
    }

    void RESTAPI_board_handler::DoPost() {
        auto id= GetBinding("id","");
        if(id.empty()) {
            return BadRequest(RESTAPI::Errors::MissingUUID);
        }

        auto RawObject = ParseStream();
        AnalyticsObjects::BoardInfo NewObject;
        if(!NewObject.from_json(RawObject)) {
            return BadRequest(RESTAPI::Errors::InvalidJSONDocument);
        }

        ProvObjects::CreateObjectInfo(RawObject,UserInfo_.userinfo,NewObject.info);

        if(StorageService()->BoardsDB().CreateRecord(NewObject)) {
            VenueCoordinator()->AddBoard(NewObject.info.id);
            AnalyticsObjects::BoardInfo NewBoard;
            StorageService()->BoardsDB().GetRecord("id",NewObject.info.id,NewBoard);
            Poco::JSON::Object  Answer;
            NewBoard.to_json(Answer);
            return ReturnObject(Answer);
        }
    }

    void RESTAPI_board_handler::DoPut() {
        auto id= GetBinding("id","");
        if(id.empty()) {
            return BadRequest(RESTAPI::Errors::MissingUUID);
        }

        AnalyticsObjects::BoardInfo Existing;
        if(!StorageService()->BoardsDB().GetRecord("id",id,Existing)) {
            return NotFound();
        }

        auto RawObject = ParseStream();
        AnalyticsObjects::BoardInfo NewObject;
        if(!NewObject.from_json(RawObject)) {
            return BadRequest(RESTAPI::Errors::InvalidJSONDocument);
        }

        ProvObjects::UpdateObjectInfo(RawObject,UserInfo_.userinfo,Existing.info);

        if(RawObject->has("venueList")) {
            if(NewObject.venueList.empty()) {
                return BadRequest("Invalid VenueList.");
            }
            Existing.venueList = NewObject.venueList;
        }

        if(StorageService()->BoardsDB().UpdateRecord("id",Existing.info.id,Existing)) {
            VenueCoordinator()->ModifyBoard(Existing.info.id);
            AnalyticsObjects::BoardInfo NewBoard;
            StorageService()->BoardsDB().GetRecord("id",Existing.info.id,NewBoard);
            Poco::JSON::Object  Answer;
            NewBoard.to_json(Answer);
            return ReturnObject(Answer);
        }
        return InternalError("Board could nto be modified. Verify and try again.");
    }
}