#include <windows.h>
#include <stdio.h>
#include <conio.h>
#include <math.h>
#include "C_vector.h"
#include "World.h"
#include "Loadase.h"
#include "Jmalloc.h"
#include "R3BspUtil.h"
#include "MeshUtil.h"
#include "jerror.h"


_HELPER *Helper;
_CAMERA *Camera;
_SCENE *Scene;


int GetParentsNum(char *p_name,_HELPER *helper)
{
	int cnt=0;
	char s_name[256];

	if( p_name == NULL || helper == NULL )
		return 0;

	strcpy(s_name,p_name);
	while( 1 )
	{
		int is_found=0;

		for(int i=0; i<helper->num; i++)
		{
			if( !strcmp(helper[i].objectname,s_name ))
			{
				is_found=1;
				cnt++;
				strcpy(s_name,helper[i].parentname);
				break;
			}
		}
		if( is_found==0 )
			break;
	}
	return cnt;
}

void SaveHelperParent(FILE *fp,char *p_name,_HELPER *helper)
{
	int is_found=0;
	char s_name[256]="";

	if( helper == NULL )
		return;
	for(int i=0; i<helper->num; i++)
	{
		if( !strcmp(helper[i].objectname,p_name ))
		{
			is_found=1;
			strcpy(s_name,helper[i].parentname);
			break;
		}
	}
	if( i == helper->num )
		return;
	if( p_name[0] != NULL )
	{
		SaveHelperParent(fp,s_name,helper);
		fwrite(helper[i].s_matrix,sizeof(D3DMATRIX),1,fp);
		fwrite(helper[i].pos,sizeof(Vector3f),1,fp);
		fwrite(helper[i].quat,sizeof(Vector4f),1,fp);
		fwrite(&helper[i].Pos_cnt,4,1,fp);
		fwrite(&helper[i].Rot_cnt,4,1,fp);
		fwrite(helper[i].Pos,sizeof(_POS_TRACK)*helper[i].Pos_cnt,1,fp);
		fwrite(helper[i].Rot,sizeof(_ROT_TRACK)*helper[i].Rot_cnt,1,fp);

		D3DMATRIX temp;
		D3DMath_MatrixFromQuaternion(temp,helper[i].quat[0],helper[i].quat[1],helper[i].quat[2],helper[i].quat[3]);
	}
}

void GetCameraFactorFromHelper(_CAMERA *camera,_HELPER *helper)	//pos���� �θ� �ι��� �ڱ���� ���ؼ� ����...
{
	int i,j;
	D3DMATRIX invmat,temp;

	for(i=0; i<camera->num; i++)
	{
		memcpy(&temp,&camera[i].d3d_trmatrix,sizeof(D3DMATRIX));
		if( camera[i].parentname[0]!= NULL)
		{
			if( helper == NULL )
				Error("ī�޶� �θ� ���۰� �ƴ�","");
			for(j=0 ;j<helper->num; j++)
			{
				if(! strcmp(helper[j].objectname,camera[i].parentname))
				{					
					D3DMath_MatrixInvert( invmat, helper[j].d3d_trmatrix );
					D3DMath_MatrixMultiply( temp,invmat, camera[i].d3d_trmatrix );
				}
			}
		}

		camera[i].pos[0] = temp._41;
		camera[i].pos[1] = temp._42;
		camera[i].pos[2] = temp._43;

		//D3DMath_QuaternionFromMatrix(camera[i].quat[0],camera[i].quat[1],camera[i].quat[2],camera[i].quat[3],temp);

		D3DMATRIX rot_m,inv_rot;
		D3DMath_MatrixFromQuaternion(rot_m,camera[i].quat[0],camera[i].quat[1],camera[i].quat[2],camera[i].quat[3]);
		//D3DMath_MatrixFromQuaternion(rot_m,helper[i].quat[0],helper[i].quat[1],helper[i].quat[2],helper[i].quat[3]);

		D3DMath_MatrixInvert( inv_rot, rot_m );
		D3DMath_MatrixMultiply( camera[i].d3d_s_matrix ,temp,inv_rot );	//������ ���� ��Ʈ����
//		D3DMath_MatrixMultiply( camera[i].d3d_s_matrix ,inv_rot,temp );	//������ ���� ��Ʈ����
//		camera[i].d3d_s_matrix._41=0;
//		camera[i].d3d_s_matrix._42=0;
//		camera[i].d3d_s_matrix._43=0;
//		camera[i].d3d_s_matrix._44=1;
		D3DMath_MatrixMultiply( temp, camera[i].d3d_s_matrix ,rot_m );	//�׽�Ʈ
	}
}

void GetHelperFactor(_HELPER *helper)	//���� �θ� �ι��� �ڱ���� ���ؼ� ����...
{
	int i,j;
	D3DMATRIX invmat,temp;

	if( helper == NULL )
		return;
	for(i=0; i<helper->num; i++)
	{
		if( helper[i].parentname[0]!= NULL)
		{
			for(j=0 ;j<helper->num; j++)
			{
				if(! strcmp(helper[j].objectname,helper[i].parentname))
				{
					
					D3DMath_MatrixInvert( invmat, helper[j].d3d_trmatrix );
					D3DMath_MatrixMultiply( temp,invmat, helper[i].d3d_trmatrix );
					break;
					//D3DMath_QuaternionFromMatrix(camera[i].quat[0],camera[i].quat[1],camera[i].quat[2],camera[i].quat[3],temp);
				}
			}
		}
		else
			memcpy(&temp,&helper[i].d3d_trmatrix,sizeof(D3DMATRIX));

		D3DMATRIX rot_m,inv_rot;
		D3DMath_MatrixFromQuaternion(rot_m,helper[i].quat[0],helper[i].quat[1],helper[i].quat[2],helper[i].quat[3]);

		D3DMath_MatrixInvert( inv_rot, rot_m );
		D3DMath_MatrixMultiply( helper[i].d3d_s_matrix ,inv_rot,temp );	//������ ���� ��Ʈ����
		helper[i].d3d_s_matrix._41=0;
		helper[i].d3d_s_matrix._42=0;
		helper[i].d3d_s_matrix._43=0;
		helper[i].d3d_s_matrix._44=1;
		//D3DMath_MatrixMultiply( temp,rot_m ,helper[i].d3d_s_matrix);	//������ ���� ��Ʈ����

		helper[i].pos[0] = temp._41;
		helper[i].pos[1] = temp._42;
		helper[i].pos[2] = temp._43;
	}
}

float version = 1.2f;

void CameraExport(char* asefile)
{
	FILE *fp;
	int i;
	char name[256]=".\\Map\\";
	char t_name[256];

	_INIFILE *Ini=GetIniFile();
	//---------- ����� BSP�����̸��� ����.
//	GetR3BspOutFileName(Ini->In_file,Ini->Out_file);	//����� BSP�����̸��� ����.

    Scene=LoadSceneASE(asefile);		// scene �ε�
    Helper=LoadHelperASE(asefile);		// ���� �ε�
    Camera=LoadCameraASE(asefile);		// ī�޶� �ε�

	if( Camera == NULL )
	{
		Error("export�� ī�޶� �����ϴ�.","");
	}

	StripPath(asefile);
	StripEXT(asefile);
	strcat(name,asefile);
	strcpy(t_name,name);
	strcat(t_name,"\\");
	strcat(t_name,asefile);
	strcat(t_name,".cam");

	GetCameraFactorFromHelper(Camera,Helper);	//���� �θ� �ι��� �ڱ���� ���ؼ� ����...
	GetHelperFactor(Helper);					//���� �θ� �ι��� �ڱ���� ���ؼ� ����...

	fp =fopen(t_name,"wb");
	fwrite(&version,4,1,fp);	//�׻� ��������..

	fwrite(&Camera->num,4,1,fp);
	fwrite(&Scene->StartFrame,4,1,fp);	//���� ������.
	fwrite(&Scene->EndFrame,4,1,fp);	//�� ������..
	// �ֻ��� �θ� ã�Ƽ� ���� ���̺�
	for(i=0; i<Camera->num; i++)
	{
		strlwr(Camera[i].objectname);	//�ҹ��ڷ� ����
		fwrite(&Camera[i].objectname,64,1,fp);
		fwrite(&Camera[i].fov,4,1,fp);
		fwrite(&Camera[i].tdist,4,1,fp);
		int num=GetParentsNum(Camera[i].parentname,Helper);
		num++;
		fwrite(&num,4,1,fp);
		SaveHelperParent(fp,Camera[i].parentname,Helper);	//�θ����..

		fwrite(&Camera[i].d3d_s_matrix,sizeof(D3DMATRIX),1,fp);
		fwrite(Camera[i].pos,sizeof(Vector3f),1,fp);
		fwrite(Camera[i].quat,sizeof(Vector4f),1,fp);
		fwrite(&Camera[i].Pos_cnt,4,1,fp);
		fwrite(&Camera[i].Rot_cnt,4,1,fp);
		fwrite(Camera[i].Pos,sizeof(_POS_TRACK)*Camera[i].Pos_cnt,1,fp);
		fwrite(Camera[i].Rot,sizeof(_ROT_TRACK)*Camera[i].Rot_cnt,1,fp);
	}


	//���� �������� ���� ����....
	DWORD h_sub_cnt;	//���� ��������...
	
	if( Helper == NULL )
		h_sub_cnt=0;
	else
		h_sub_cnt = (DWORD)Helper->num;

	fwrite(&h_sub_cnt,4,1,fp);
	for( i=0; i<(int)h_sub_cnt; i++)
	{
		fwrite(Helper[i].objectname,64,1,fp);	//�ڽ��� �̸��� �θ��� �̸��� �ִ´�.
		fwrite(Helper[i].parentname,64,1,fp);	
		fwrite(Helper[i].s_matrix,sizeof(D3DMATRIX),1,fp);
		fwrite(Helper[i].pos,sizeof(Vector3f),1,fp);
		fwrite(Helper[i].quat,sizeof(Vector4f),1,fp);
		fwrite(&Helper[i].Pos_cnt,4,1,fp);
		fwrite(&Helper[i].Rot_cnt,4,1,fp);
		fwrite(Helper[i].Pos,sizeof(_POS_TRACK)*Helper[i].Pos_cnt,1,fp);
		fwrite(Helper[i].Rot,sizeof(_ROT_TRACK)*Helper[i].Rot_cnt,1,fp);
	}

	fclose(fp);
}


void main(int argc, char **argv)
{
//-------------------- Ase�� ����â���� �ҷ��´�.
	static OPENFILENAME ofn; // zero the ofn members out
    char ase_name[256]="1.map";

	ofn.lStructSize = sizeof(ofn);
	ofn.lpstrFile = ase_name;
	ofn.nMaxFile = sizeof(ase_name);
	ofn.lpstrFilter = "R3 MAP (*.MAP)\0*.MAP\0All (*.*)\0*.*\0";
	ofn.lpstrTitle = "Select R3 MAP file";
	ofn.Flags = OFN_NOCHANGEDIR | OFN_HIDEREADONLY |
               OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;


	if (!GetOpenFileName(&ofn))
		return;

	strcpy(GetIniFile()->In_file,ase_name);
//-------------------------------------------------
	CameraExport(ase_name);

	printf("Complete!\n");
	printf("Press any key to continue\n");
	getch();
}
