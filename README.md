<p align="center">
    <img src="images/project/logo.svg" width="200"/>
</p>

# OpenWiFI Analytics Service (OWANALYTICS)

## What is it?
The OpenWiFi Analytics Service is a service for the TIP OpenWiFi CloudSDK (OWSDK).
OWANALYTICS gathers statistics about device used in OpenWiFI and groups them according to their 
provisioning (OWPROV) entities or venues. OWANALYTICS, like all other OWSDK microservices, is
defined using an OpenAPI definition and uses the ucentral communication protocol to interact with Access Points. To use
the OWANALYTICS, you either need to [build it](#building) or use the [Docker version](#docker).

## Building
To build the microservice from source, please follow the instructions in [here](./BUILDING.md)

## Docker
To use the CLoudSDK deployment please follow [here](https://github.com/Telecominfraproject/wlan-cloud-ucentral-deploy)

### OWANALYTICS Service Configuration
The configuration is kept in a file called `owanalytics.properties`. To understand the content of this file,
please look [here](https://github.com/Telecominfraproject/wlan-cloud-analytics/blob/main/CONFIGURATION.md)

## OpenAPI
You may get static page with OpenAPI docs generated from the definition on [GitHub Page](https://telecominfraproject.github.io/wlan-cloud-analytics/).

Also, you may use [Swagger UI](https://petstore.swagger.io/#/) with OpenAPI definition file raw link (i.e. [latest version file](https://raw.githubusercontent.com/Telecominfraproject/wlan-cloud-analytics/main/openapi/owanalytics.yaml)) to get interactive docs page.

## Firewall Considerations
| Port  | Description                                    | Configurable |
|:------|:-----------------------------------------------|:------------:|
| 16005 | Default port for REST API Access to the OWPROV |     yes      |

## Kafka topics
Toe read more about Kafka, follow the [document](https://github.com/Telecominfraproject/wlan-cloud-ucentralgw/blob/main/KAFKA.md)

## Contributions
We need more contributors. Should you wish to contribute,
please follow the [contributions](https://github.com/Telecominfraproject/wlan-cloud-ucentralgw/blob/master/CONTRIBUTING.md) document.

## Pull Requests
Please create a branch with the Jira addressing the issue you are fixing or the feature you are implementing.
Create a pull-request from the branch into master.

## Additional OWSDK Microservices
Here is a list of additional OWSDK microservices
| Name | Description | Link | OpenAPI |
| :--- | :--- | :---: | :---: |
| OWSEC | Security Service | [here](https://github.com/Telecominfraproject/wlan-cloud-ucentralsec) | [here](https://github.com/Telecominfraproject/wlan-cloud-ucentralsec/blob/main/openpapi/owsec.yaml) |
| OWGW | Controller Service | [here](https://github.com/Telecominfraproject/wlan-cloud-ucentralgw) | [here](https://github.com/Telecominfraproject/wlan-cloud-ucentralgw/blob/master/openapi/owgw.yaml) |
| OWFMS | Firmware Management Service | [here](https://github.com/Telecominfraproject/wlan-cloud-ucentralfms) | [here](https://github.com/Telecominfraproject/wlan-cloud-ucentralfms/blob/main/openapi/owfms.yaml) |
| OWPROV | Provisioning Service | [here](https://github.com/Telecominfraproject/wlan-cloud-owprov) | [here](https://github.com/Telecominfraproject/wlan-cloud-owprov/blob/main/openapi/owprov.yaml) |
| OWANALYTICS | Analytics Service | [here](https://github.com/Telecominfraproject/wlan-cloud-analytics) | [here](https://github.com/Telecominfraproject/wlan-cloud-analytics/blob/main/openapi/owanalytics.yaml) |
| OWSUB | Subscriber Service | [here](https://github.com/Telecominfraproject/wlan-cloud-userportal) | [here](https://github.com/Telecominfraproject/wlan-cloud-userportal/blob/main/openapi/userportal.yaml) |

