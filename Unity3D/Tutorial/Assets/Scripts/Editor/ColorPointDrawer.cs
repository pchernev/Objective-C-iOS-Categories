using UnityEngine;
using UnityEditor;

[CustomPropertyDrawer(typeof(ColorPoint))]
public class ColorPointDrawer : PropertyDrawer
{
	public override float GetPropertyHeight( SerializedProperty property, GUIContent label )
	{
		return property.isExpanded ? 32F : 16F;
	}
	
	public override void OnGUI (Rect position, SerializedProperty property, GUIContent label) {
		position.height = 16F;
		Rect foldoutPosition = position;
		foldoutPosition.x -= 14f;
		foldoutPosition.width += 14f;
		label = EditorGUI.BeginProperty( position, label, property );
		property.isExpanded = EditorGUI.Foldout(foldoutPosition, property.isExpanded, label, true);
		EditorGUI.EndProperty();
		
		if( !property.isExpanded )
			return;
	
		position = EditorGUI.IndentedRect( position );
		position.y += 16F;		
		position.width /= 4F;
		int oldIndentLevel = EditorGUI.indentLevel;
		EditorGUI.indentLevel = 0;
		EditorGUIUtility.LookLikeControls( 12F );
		EditorGUI.PropertyField( position, property.FindPropertyRelative( "position.x" ));
		position.x += position.width;
		EditorGUI.PropertyField( position, property.FindPropertyRelative( "position.y" ));
		position.x += position.width;
		EditorGUI.PropertyField( position, property.FindPropertyRelative( "position.z" ));
		position.x += position.width;		
		EditorGUI.PropertyField( position, property.FindPropertyRelative( "color" ));
		EditorGUI.indentLevel = oldIndentLevel;
	}
}
